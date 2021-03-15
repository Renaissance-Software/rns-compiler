#include <RNS/types.h>
#include <RNS/arch.h>
#include <RNS/compiler.h>
#include <RNS/os.h>
#include <RNS/os_internal.h>

#include "pe32.h"

using RNS::Allocator;
using RNS::DebugAllocator;

struct FileBuffer
{
    u8* ptr;
    s64 len;
    s64 cap;

    static FileBuffer create(Allocator* allocator)
    {
        FileBuffer ab =
        {
            .ptr = reinterpret_cast<u8*>(allocator->pool.ptr + allocator->pool.used),
            .len = 0,
            .cap = (allocator->pool.cap - allocator->pool.used) / (s64)sizeof(u8),
        };

        return ab;
    }

    template <typename T>
    T* append(T element) {
        s64 size = sizeof(T);
        assert(len + size < cap);
        auto index = len;
        memcpy(&ptr[index], &element, size);
        len += size;
        return reinterpret_cast<T*>(&ptr[index]);
    }

    template <typename T>
    T* append(T* element)
    {
        s64 size = sizeof(T);
        assert(len + size < cap);
        auto index = len;
        memcpy(&ptr[index], element, size);
        len += size;
        return reinterpret_cast<T*>(&ptr[index]);
    }

    template<typename T>
    T* append(T* arr, s64 array_len)
    {
        s64 size = sizeof(T) * array_len;
        assert(len + size < cap);
        auto index = len;
        memcpy(&ptr[index], arr, size);
        len += size;
        return reinterpret_cast<T*>(&ptr[index]);
    }
};

enum class ImageDirectoryIndex
{
    Export,
    Import,
    Resource,
    Exception,
    Security,
    Relocation,
    Debug,
    Architecture,
    GlobalPtr,
    TLS,
    LoadConfig,
    BoundImport,
    IAT,
    Delay,
    CLR,
};

#define file_offset_to_rva(_section_header_)\
    (static_cast<DWORD>(file_buffer.len) - (_section_header_)->PointerToRawData + (_section_header_)->VirtualAddress)

struct ImportNameTo_RVA
{
    const char* name;
    struct
    {
        u32 name;
        u32 IAT;
    } RVA;
};

struct ImportLibrary
{
    ImportNameTo_RVA dll;
    ImportNameTo_RVA* functions;
    u32 image_thunk_RVA;
    s32 function_count;
};


void write_executable()
{
    DebugAllocator file_allocator = create_allocator(RNS_MEGABYTE(1));
    FileBuffer file_buffer = FileBuffer::create(&file_allocator);
    IMAGE_DOS_HEADER dos_header
    {
        .e_magic = IMAGE_DOS_SIGNATURE,
        .e_lfanew = sizeof(IMAGE_DOS_HEADER),
    };
    file_buffer.append(&dos_header);
    file_buffer.append((u32)IMAGE_NT_SIGNATURE);

    IMAGE_FILE_HEADER image_file_header = {
        .Machine = IMAGE_FILE_MACHINE_AMD64,
        .NumberOfSections = 2,
        .TimeDateStamp = 0x5EF48E56,
        .SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64),
        .Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LARGE_ADDRESS_AWARE,
    };
    file_buffer.append(&image_file_header);

    IMAGE_OPTIONAL_HEADER64 oh = {
        .Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC,
        .SizeOfCode = 0x200, // @TODO: change
        .SizeOfInitializedData = 0x200, // @TODO: change (global data)
        .AddressOfEntryPoint = 0x2000, // @TODO: change
        .BaseOfCode = 0x2000,// @TODO: change
        .ImageBase = 0x0000000140000000,
        .SectionAlignment = 0x1000,
        .FileAlignment = 0x200,
        .MajorOperatingSystemVersion = 6, // @TODO: change
        .MinorOperatingSystemVersion = 0, // @TODO: change
        .MajorSubsystemVersion = 6,
        .MinorSubsystemVersion = 0,
        .SizeOfImage = 0x3000,
        .SizeOfHeaders = 0,
        .CheckSum = 0,
        .Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI,
        .DllCharacteristics = IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA | IMAGE_DLLCHARACTERISTICS_NX_COMPAT | IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE | IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE,
        .SizeOfStackReserve = 0x100000,
        .SizeOfStackCommit = 0x1000,
        .SizeOfHeapReserve = 0x100000,
        .SizeOfHeapCommit = 0x1000,
        .NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES,
    };
    auto* optional_header = file_buffer.append(&oh);

    IMAGE_SECTION_HEADER rdsh = {
      .Name = ".rdata",
      .Misc = 0x14c,
      .VirtualAddress = 0x1000, // calculate this
      .SizeOfRawData = 0x200, // calculate this
      .PointerToRawData = 0,
      .Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ,
    };
    auto* rdata_section_header = file_buffer.append(&rdsh);

    IMAGE_SECTION_HEADER tsh = {
        .Name = ".text",
        .Misc = {.VirtualSize = 0x10}, // sizeof machine code in bytes
        .VirtualAddress = optional_header->BaseOfCode, // calculate this
        .SizeOfRawData = optional_header->SizeOfCode, // calculate this
        .PointerToRawData = 0,
        .Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE,
    };
    auto* text_section_header = file_buffer.append(&tsh);

    // end list with a NULL header
    file_buffer.append(IMAGE_SECTION_HEADER{});

    optional_header->SizeOfHeaders = static_cast<DWORD>(RNS::align(file_buffer.len, optional_header->FileAlignment));
    file_buffer.len = optional_header->SizeOfHeaders;

    IMAGE_SECTION_HEADER* sections[] = { rdata_section_header, text_section_header };
    auto section_offset = optional_header->SizeOfHeaders;
    for (auto i = 0; i < rns_array_length(sections); i++)
    {
        sections[i]->PointerToRawData = section_offset;
        section_offset += sections[i]->SizeOfRawData;
    }


    ImportNameTo_RVA kernel32_functions[] = {
        { .name = "ExitProcess", .RVA = { .name = 0xcccccccc, .IAT = 0xcccccccc } },
        { .name = "GetStdHandle", .RVA = { .name = 0xcccccccc, .IAT = 0xcccccccc } },
        { .name = "ReadConsoleA", .RVA = { .name = 0xcccccccc, .IAT = 0xcccccccc } },
    };

    ImportNameTo_RVA user32_functions[] = {
        { .name = "ShowWindow", .RVA = { .name = 0xcccccccc, .IAT = 0xcccccccc } },
    };

    ImportLibrary import_libraries[] = {
        {
            .dll = { .name = "kernel32.dll", .RVA = {.name = 0xcccccccc, .IAT = 0xcccccccc } },
            .functions = kernel32_functions,
            .image_thunk_RVA = 0xcccccccc,
            .function_count = rns_array_length(kernel32_functions),
        },
        {
            .dll = { .name = "user32.dll", .RVA = {.name = 0xcccccccc, .IAT = 0xcccccccc } },
            .functions = user32_functions,
            .image_thunk_RVA = 0xcccccccc,
            .function_count = rns_array_length(user32_functions),
        }
    };

    // IAT
    file_buffer.len = rdata_section_header->PointerToRawData;

    for (auto i = 0; i < rns_array_length(import_libraries); i++)
    {
        ImportLibrary* lib = &import_libraries[i];
        for (auto i = 0; i < lib->function_count; i++)
        {
            lib->functions[i].RVA.name = file_offset_to_rva(rdata_section_header);
            file_buffer.append((s16)0);
            auto name_size = strlen(lib->functions[i].name) + 1;
            auto aligned_name_size = RNS::align(name_size, 2);
            file_buffer.append(lib->functions[i].name, name_size);
            file_buffer.len += aligned_name_size - name_size;
        }
    }

    optional_header->DataDirectory[static_cast<u32>(ImageDirectoryIndex::Import)].VirtualAddress = file_offset_to_rva(rdata_section_header);

    for (auto i = 0; i < rns_array_length(import_libraries); i++)
    {
        ImportLibrary* lib = &import_libraries[i];
        lib->dll.RVA.IAT = file_offset_to_rva(rdata_section_header);

        for (auto i = 0; i < lib->function_count; i++)
        {
            ImportNameTo_RVA* fn = &lib->functions[i];
            fn->RVA.IAT = file_offset_to_rva(rdata_section_header);
            file_buffer.append((u64)lib->functions[i].RVA.name);
        }

        // End of list (sentinel)
        file_buffer.append((u64)0);
    }
    optional_header->DataDirectory[static_cast<u32>(ImageDirectoryIndex::Import)].Size = static_cast<DWORD>(file_buffer.len) - rdata_section_header->PointerToRawData;

    for (auto i = 0; i < rns_array_length(import_libraries); i++)
    {
        ImportLibrary* lib = &import_libraries[i];
        lib->image_thunk_RVA = file_offset_to_rva(rdata_section_header);

        for (auto i = 0; i < lib->function_count; i++)
        {
            file_buffer.append((u64)lib->functions[i].RVA.name);
        }

        // Sentinel
        file_buffer.append((u64)0);
    }

    // Sentinel
    file_buffer.append((u64)0);

    for (auto i = 0; i < rns_array_length(import_libraries); i++)
    {
        ImportLibrary* lib = &import_libraries[i];
        lib->dll.RVA.name = file_offset_to_rva(rdata_section_header);
        
        auto name_size = strlen(lib->dll.name) + 1;
        auto aligned_name_size = RNS::align(name_size, 2);
        file_buffer.append(lib->dll.name, name_size);
        file_buffer.len += aligned_name_size - name_size;
    }

    auto import_directory_RVA = file_offset_to_rva(rdata_section_header);
    optional_header->DataDirectory[static_cast<u32>(ImageDirectoryIndex::Import)].VirtualAddress = static_cast<DWORD>(import_directory_RVA);

    for (auto i = 0; i < rns_array_length(import_libraries); i++)
    {
        ImportLibrary* lib = &import_libraries[i];

        file_buffer.append(IMAGE_IMPORT_DESCRIPTOR {
            .OriginalFirstThunk = lib->image_thunk_RVA,
            .Name = lib->dll.RVA.name,
            .FirstThunk = lib->dll.RVA.IAT,
        });
    }

    optional_header->DataDirectory[static_cast<u32>(ImageDirectoryIndex::Import)].Size = file_offset_to_rva(rdata_section_header) - import_directory_RVA;

    file_buffer.append(IMAGE_IMPORT_DESCRIPTOR{});

    // useless statement
    file_buffer.len = (u64)rdata_section_header->PointerToRawData + (u64)rdata_section_header->SizeOfRawData;

    // .text section
    file_buffer.len = text_section_header->PointerToRawData;

    u8 sub_rsp_28_bytes[] = { 0x48, 0x83, 0xEC, 0x28 };
    file_buffer.append(sub_rsp_28_bytes, rns_array_length(sub_rsp_28_bytes));

    u8 mov_rcx_42_bytes[] = { 0xB9, 0x2A, 0x00, 0x00, 0x00 };
    file_buffer.append(mov_rcx_42_bytes, rns_array_length(mov_rcx_42_bytes));

    u8 call_prologue_bytes[] = { 0xFF, 0x15, };
    file_buffer.append(call_prologue_bytes, rns_array_length(call_prologue_bytes));

    {
        u32* exit_process_rip_relative_address = file_buffer.append((u32)0);
        u64 exit_process_call_rva = file_offset_to_rva(text_section_header);

        bool match_found = false;
        for (auto i = 0; !match_found && i < rns_array_length(import_libraries); i++)
        {
            ImportLibrary* lib = &import_libraries[i];

            if (strcmp(lib->dll.name, "kernel32.dll") != 0) continue;

            for (auto i = 0; i < lib->function_count; i++)
            {
                ImportNameTo_RVA* fn = &lib->functions[i];

                match_found = strcmp(fn->name, "ExitProcess") == 0;
                if (match_found)
                {
                    *exit_process_rip_relative_address = static_cast<u32>(fn->RVA.IAT - exit_process_call_rva);
                    break;
                }
            }
        }

        assert(match_found);
    }

    u8 int3_bytes[] = { 0xCC };
    file_buffer.append(int3_bytes, rns_array_length(int3_bytes));

    file_buffer.len = text_section_header->PointerToRawData + text_section_header->SizeOfRawData;

    {
        auto file_handle = CreateFileA("test.exe", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        assert(file_handle != INVALID_HANDLE_VALUE);

        DWORD bytes_written = 0;
        WriteFile(file_handle, file_buffer.ptr, (DWORD)file_buffer.len, &bytes_written, 0);
        CloseHandle(file_handle);
    }
}
