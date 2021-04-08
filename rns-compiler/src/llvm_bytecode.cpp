#include "llvm_bytecode.h"

#include <RNS/profiler.h>
#include <stdio.h>

namespace RNS
{
    struct SlotTracker;
    struct BasicBlock;
    struct Function;
    struct Instruction;
    struct Module;
    using BasicBlockBuffer = Buffer<BasicBlock>;
    using InstructionBuffer = Buffer<Instruction>;
    using namespace AST;
    using IDType = u64;

    enum class TypeID
    {
        Void,
        Label,
        Integer,
        Float,
        Pointer,
        Vector,
        Struct,
        Array,
        Function,
    };

    enum class ValueID : u8
    {
        Undefined = 0,
        Argument,
        BasicBlock,
        InlineASM,
        Metadata,
        BlockAddress,
        ConstantArray,
        ConstantStruct,
        ConstantVector,
        ConstantAggregateZero,
        ConstantDataArray,
        ConstantDataVector,
        ConstantFP,
        ConstantInt,
        ConstantPointerNull,
        ConstExprBinary, // not exposed
        ConstExprCompare,
        ConstExprExtractElement,
        ConstExprExtractValue,
        ConstExprGetElementPtr,
        ConstExprInsertElement,
        ConstExprInsertValue,
        ConstExprSelect,
        ConstExprShuffleVector,
        ConstExprUnary,
        GlobalIndirectFunction,
        GlobalIndirectAlias,
        GlobalFunction,
        GlobalVariable,
        MemoryAccess,
        Instruction,
        Operator,
        OperatorAddrSpaceCast,
        OperatorBitCast,
        OperatorGEP,
        OperatorPointerToInt,
        OperatorZeroExtend,
        OperatorFPMathOperator,
        // @TODO: figure out Signed-wrapping
    };

    struct Type
    {
        StringView name;
        TypeID id;
    };
    using TypeRefBuffer = Buffer<Type*>;

    struct Value
    {
        RNS::Type* type;
        ValueID base_id;
        u8 padding[7];

        const char* print(char* buffer, SlotTracker& slot_tracker);
    };

    struct FloatType
    {
        Type base;
        u16 bits;
    };

    struct IntegerType
    {
        Type base;
        u16 bits;
    };

    struct PointerType
    {
        Type base;
        Type* type;
    };

    struct StructType
    {
        Type base;
        Slice<Type*> field_types;
    };

    struct ArrayType
    {
        Type base;
        Type* type;
        u64 count;
    };

    struct FunctionType
    {
        Type base;
        Slice<Type*> arg_types;
        Type* ret_type;
    };

    struct ConstantArray
    {
        Value value;
        Type* array_type;
        Slice<Value*> array_values;
    };

    struct Intrinsic
    {
    };

    struct Context
    {
        Type void_type, label_type;
        IntegerType i1, i8, i16, i32, i64;
        FloatType f32, f64;
        // @TODO: add vector types and custom types

        Buffer<FunctionType> function_types;
        Buffer<ArrayType> array_types;
        Buffer<PointerType> pointer_types;
        Buffer<ConstantArray> constant_arrays;

        static Context create(Allocator* allocator)
        {
            auto get_base_type = [](TypeID type, const char* name)
            {
                Type base = {
                    .name = StringView::create(name, strlen(name)),
                    .id = type,
                };

                return base;
            };

            Context context = {};
            context.void_type = get_base_type(TypeID::Void, "void");
            context.label_type = get_base_type(TypeID::Label, "label");

            auto get_integer_type = [&](u16 bits, const char* name)
            {
                IntegerType integer_type = {
                    .base = get_base_type(TypeID::Integer, name),
                    .bits = bits,
                };

                return integer_type;
            };

            context.i1 = get_integer_type(1, "i1");
            context.i8 = get_integer_type(8, "i8");
            context.i16 = get_integer_type(16, "i16");
            context.i32 = get_integer_type(32, "i32");
            context.i64 = get_integer_type(64, "i64");

            auto get_float_type = [&](u16 bits, const char* name)
            {
                FloatType float_type = {
                    .base = get_base_type(TypeID::Integer, name),
                    .bits = bits,
                };

                return float_type;
            };

            context.f32 = get_float_type(32, "f32");
            context.f64 = get_float_type(64, "f64");

            context.function_types = context.function_types.create(allocator, 1024);
            context.array_types = context.array_types.create(allocator, 1024);
            context.pointer_types = context.pointer_types.create(allocator, 1024);
            context.constant_arrays = context.constant_arrays.create(allocator, 1024);


            return context;
        }

        Type* get_void_type()
        {
            return &void_type;
        }

        Type* get_label_type()
        {
            return &label_type;
        }

        Type* get_boolean_type()
        {
            return reinterpret_cast<Type*>(&i1);
        }

        Type* get_integer_type(u16 bits)
        {
            switch (bits)
            {
                case 1:
                    return reinterpret_cast<Type*>(&i1);
                case 8:
                    return reinterpret_cast<Type*>(&i8);
                case 16:
                    return reinterpret_cast<Type*>(&i16);
                case 32:
                    return reinterpret_cast<Type*>(&i32);
                case 64:
                    return reinterpret_cast<Type*>(&i64);
                default:
                    RNS_NOT_IMPLEMENTED;
            }

            return nullptr;
        }

        Type* get_float_type(u16 bits)
        {
            switch (bits)
            {
                case 32:
                    return reinterpret_cast<Type*>(&f32);
                case 64:
                    return reinterpret_cast<Type*>(&f64);
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }

            return nullptr;
        }

        Type* get_pointer_type(Type* type)
        {
            assert(type);

            for (auto& pt_type : pointer_types)
            {
                if (pt_type.type == type)
                {
                    return reinterpret_cast<Type*>(&pt_type);
                }
            }

            PointerType* pointer_type = pointer_types.allocate();
            pointer_type->base.id = TypeID::Pointer;
            pointer_type->type = type;

            return reinterpret_cast<Type*>(pointer_type);
        }

        ConstantArray* get_constant_array(Slice<Value*> values, Type* type)
        {
            ConstantArray* constarray = constant_arrays.allocate();
            constarray->value.base_id = ValueID::ConstantArray;
            constarray->array_type = type;
            constarray->array_values = values;

            return constarray;
        }
    };

    Type* get_type(Allocator* allocator, Context& context, User::Type* type)
    {
        assert(type);
        switch (type->id)
        {
            case User::TypeID::FunctionType:
            {
                Type* ret_type = get_type(allocator, context, type->function_t.ret_type);
                assert(ret_type);
                auto arg_count = type->function_t.arg_types.len;

                for (auto& fn_type : context.function_types)
                {
                    if (ret_type != fn_type.ret_type)
                    {
                        continue;
                    }
                    for (auto i = 0; i < type->function_t.arg_types.len; i++)
                    {
                        auto* user_arg_type = type->function_t.arg_types[i];
                        auto* arg_type = get_type(allocator, context, user_arg_type);
                        auto* fn_arg_type = fn_type.arg_types[i];
                        if (arg_type == fn_arg_type)
                        {
                            continue;
                        }
                        break;
                    }

                    return reinterpret_cast<Type*>(&fn_type);
                }

                FunctionType* function_type = context.function_types.allocate();
                function_type->base.id = TypeID::Function;
                if (arg_count)
                {
                    function_type->arg_types.ptr = new(allocator) Type * [arg_count];
                    function_type->arg_types.len = arg_count;
                }
                function_type->ret_type = ret_type;

                return reinterpret_cast<Type*>(function_type);
            } break;
            case User::TypeID::IntegerType:
            {
                auto bits = type->integer_t.bits;
                return context.get_integer_type(bits);
            } break;
            case User::TypeID::ArrayType:
            {
                auto count = type->array_t.count;
                auto* elem_type = get_type(allocator, context, type->array_t.type);
                for (auto& array_type : context.array_types)
                {
                    if (array_type.count == count && array_type.type == elem_type)
                    {
                        return reinterpret_cast<Type*>(&array_type);
                    }
                }

                ArrayType* array_type = context.array_types.allocate();
                array_type->base.id = TypeID::Array;
                array_type->count = count;
                array_type->type = elem_type;

                return reinterpret_cast<Type*>(array_type);
            } break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }
        return nullptr;
    }

    /*
    Unary instruction,
    unary operator
    binary operator,
    cast instruction,
    cmp instruction,
    call instruction, (intrinsics in LLVM are based of this)
        -instrinsic

    */

    enum class InstructionID : u8
    {
        // Terminator
        Ret = 1,
        Br = 2,
        Switch_ = 3,
        Indirectbr = 4,
        Invoke = 5,
        Resume = 6,
        Unreachable = 7,
        Cleanup_ret = 8,
        Catch_ret = 9,
        Catch_switch = 10,
        Call_br = 11,

        // Unary
        Fneg = 12,

        // Binary
        Add = 13,
        Fadd = 14,
        Sub = 15,
        Fsub = 16,
        Mul = 17,
        Fmul = 18,
        Udiv = 19,
        Sdiv = 20,
        Fdiv = 21,
        Urem = 22,
        Srem = 23,
        Frem = 24,

        // Logical
        Shl = 25,
        Lshr = 26,
        Ashr = 27,
        And = 28,
        Or = 29,
        Xor = 30,

        // Memory
        Alloca = 31,
        Load = 32,
        Store = 33,
        GetElementPtr = 34,
        Fence = 35,
        AtomicCmpXchg = 36,
        AtomicRMW = 37,

        // Cast
        Trunc = 38,
        ZExt = 39,
        SExt = 40,
        FPToUI = 41,
        FPToSI = 42,
        UIToFP = 43,
        SIToFP = 44,
        FPTrunc = 45,
        FPExt = 46,
        PtrToInt = 47,
        IntToPtr = 48,
        BitCast = 49,
        AddrSpaceCast = 50,

        // FuncLetPad
        CleanupPad = 51,
        CatchPad = 52,

        // Other
        ICmp = 53,
        FCmp = 54,
        Phi = 55,
        Call = 56,
        Select = 57,
        UserOp1 = 58,
        UserOp2 = 59,
        VAArg = 60,
        ExtractElement = 61,
        InsertElement = 62,
        ShuffleVector = 63,
        ExtractValue = 64,
        InsertValue = 65,
        LandingPad = 66,
        Freeze = 67,
    };

    // Intrinsics are instances of the call instruction in LLVM
    // Intrinsics enums for x86-64
    enum class IntrinsicID
    {
        addressofreturnaddress = 1,                    // llvm.addressofreturnaddress
        adjust_trampoline,                         // llvm.adjust.trampoline
        annotation,                                // llvm.annotation
        assume,                                    // llvm.assume
        bitreverse,                                // llvm.bitreverse
        bswap,                                     // llvm.bswap
        call_preallocated_arg,                     // llvm.call.preallocated.arg
        call_preallocated_setup,                   // llvm.call.preallocated.setup
        call_preallocated_teardown,                // llvm.call.preallocated.teardown
        canonicalize,                              // llvm.canonicalize
        ceil,                                      // llvm.ceil
        clear_cache,                               // llvm.clear_cache
        codeview_annotation,                       // llvm.codeview.annotation
        convert_from_fp16,                         // llvm.convert.from.fp16
        convert_to_fp16,                           // llvm.convert.to.fp16
        copysign,                                  // llvm.copysign
        coro_alloc,                                // llvm.coro.alloc
        coro_alloca_alloc,                         // llvm.coro.alloca.alloc
        coro_alloca_free,                          // llvm.coro.alloca.free
        coro_alloca_get,                           // llvm.coro.alloca.get
        coro_begin,                                // llvm.coro.begin
        coro_destroy,                              // llvm.coro.destroy
        coro_done,                                 // llvm.coro.done
        coro_end,                                  // llvm.coro.end
        coro_frame,                                // llvm.coro.frame
        coro_free,                                 // llvm.coro.free
        coro_id,                                   // llvm.coro.id
        coro_id_retcon,                            // llvm.coro.id.retcon
        coro_id_retcon_once,                       // llvm.coro.id.retcon.once
        coro_noop,                                 // llvm.coro.noop
        coro_param,                                // llvm.coro.param
        coro_prepare_retcon,                       // llvm.coro.prepare.retcon
        coro_promise,                              // llvm.coro.promise
        coro_resume,                               // llvm.coro.resume
        coro_save,                                 // llvm.coro.save
        coro_size,                                 // llvm.coro.size
        coro_subfn_addr,                           // llvm.coro.subfn.addr
        coro_suspend,                              // llvm.coro.suspend
        coro_suspend_retcon,                       // llvm.coro.suspend.retcon
        cos,                                       // llvm.cos
        ctlz,                                      // llvm.ctlz
        ctpop,                                     // llvm.ctpop
        cttz,                                      // llvm.cttz
        dbg_addr,                                  // llvm.dbg.addr
        dbg_declare,                               // llvm.dbg.declare
        dbg_label,                                 // llvm.dbg.label
        dbg_value,                                 // llvm.dbg.value
        debugtrap,                                 // llvm.debugtrap
        donothing,                                 // llvm.donothing
        eh_dwarf_cfa,                              // llvm.eh.dwarf.cfa
        eh_exceptioncode,                          // llvm.eh.exceptioncode
        eh_exceptionpointer,                       // llvm.eh.exceptionpointer
        eh_recoverfp,                              // llvm.eh.recoverfp
        eh_return_i32,                             // llvm.eh.return.i32
        eh_return_i64,                             // llvm.eh.return.i64
        eh_sjlj_callsite,                          // llvm.eh.sjlj.callsite
        eh_sjlj_functioncontext,                   // llvm.eh.sjlj.functioncontext
        eh_sjlj_longjmp,                           // llvm.eh.sjlj.longjmp
        eh_sjlj_lsda,                              // llvm.eh.sjlj.lsda
        eh_sjlj_setjmp,                            // llvm.eh.sjlj.setjmp
        eh_sjlj_setup_dispatch,                    // llvm.eh.sjlj.setup.dispatch
        eh_typeid_for,                             // llvm.eh.typeid.for
        eh_unwind_init,                            // llvm.eh.unwind.init
        exp,                                       // llvm.exp
        exp2,                                      // llvm.exp2
        expect,                                    // llvm.expect
        expect_with_probability,                   // llvm.expect.with.probability
        experimental_constrained_ceil,             // llvm.experimental.constrained.ceil
        experimental_constrained_cos,              // llvm.experimental.constrained.cos
        experimental_constrained_exp,              // llvm.experimental.constrained.exp
        experimental_constrained_exp2,             // llvm.experimental.constrained.exp2
        experimental_constrained_fadd,             // llvm.experimental.constrained.fadd
        experimental_constrained_fcmp,             // llvm.experimental.constrained.fcmp
        experimental_constrained_fcmps,            // llvm.experimental.constrained.fcmps
        experimental_constrained_fdiv,             // llvm.experimental.constrained.fdiv
        experimental_constrained_floor,            // llvm.experimental.constrained.floor
        experimental_constrained_fma,              // llvm.experimental.constrained.fma
        experimental_constrained_fmul,             // llvm.experimental.constrained.fmul
        experimental_constrained_fmuladd,          // llvm.experimental.constrained.fmuladd
        experimental_constrained_fpext,            // llvm.experimental.constrained.fpext
        experimental_constrained_fptosi,           // llvm.experimental.constrained.fptosi
        experimental_constrained_fptoui,           // llvm.experimental.constrained.fptoui
        experimental_constrained_fptrunc,          // llvm.experimental.constrained.fptrunc
        experimental_constrained_frem,             // llvm.experimental.constrained.frem
        experimental_constrained_fsub,             // llvm.experimental.constrained.fsub
        experimental_constrained_llrint,           // llvm.experimental.constrained.llrint
        experimental_constrained_llround,          // llvm.experimental.constrained.llround
        experimental_constrained_log,              // llvm.experimental.constrained.log
        experimental_constrained_log10,            // llvm.experimental.constrained.log10
        experimental_constrained_log2,             // llvm.experimental.constrained.log2
        experimental_constrained_lrint,            // llvm.experimental.constrained.lrint
        experimental_constrained_lround,           // llvm.experimental.constrained.lround
        experimental_constrained_maximum,          // llvm.experimental.constrained.maximum
        experimental_constrained_maxnum,           // llvm.experimental.constrained.maxnum
        experimental_constrained_minimum,          // llvm.experimental.constrained.minimum
        experimental_constrained_minnum,           // llvm.experimental.constrained.minnum
        experimental_constrained_nearbyint,        // llvm.experimental.constrained.nearbyint
        experimental_constrained_pow,              // llvm.experimental.constrained.pow
        experimental_constrained_powi,             // llvm.experimental.constrained.powi
        experimental_constrained_rint,             // llvm.experimental.constrained.rint
        experimental_constrained_round,            // llvm.experimental.constrained.round
        experimental_constrained_roundeven,        // llvm.experimental.constrained.roundeven
        experimental_constrained_sin,              // llvm.experimental.constrained.sin
        experimental_constrained_sitofp,           // llvm.experimental.constrained.sitofp
        experimental_constrained_sqrt,             // llvm.experimental.constrained.sqrt
        experimental_constrained_trunc,            // llvm.experimental.constrained.trunc
        experimental_constrained_uitofp,           // llvm.experimental.constrained.uitofp
        experimental_deoptimize,                   // llvm.experimental.deoptimize
        experimental_gc_relocate,                  // llvm.experimental.gc.relocate
        experimental_gc_result,                    // llvm.experimental.gc.result
        experimental_gc_statepoint,                // llvm.experimental.gc.statepoint
        experimental_guard,                        // llvm.experimental.guard
        experimental_patchpoint_i64,               // llvm.experimental.patchpoint.i64
        experimental_patchpoint_void,              // llvm.experimental.patchpoint.void
        experimental_stackmap,                     // llvm.experimental.stackmap
        experimental_vector_reduce_add,            // llvm.experimental.vector.reduce.add
        experimental_vector_reduce_and,            // llvm.experimental.vector.reduce.and
        experimental_vector_reduce_fmax,           // llvm.experimental.vector.reduce.fmax
        experimental_vector_reduce_fmin,           // llvm.experimental.vector.reduce.fmin
        experimental_vector_reduce_mul,            // llvm.experimental.vector.reduce.mul
        experimental_vector_reduce_or,             // llvm.experimental.vector.reduce.or
        experimental_vector_reduce_smax,           // llvm.experimental.vector.reduce.smax
        experimental_vector_reduce_smin,           // llvm.experimental.vector.reduce.smin
        experimental_vector_reduce_umax,           // llvm.experimental.vector.reduce.umax
        experimental_vector_reduce_umin,           // llvm.experimental.vector.reduce.umin
        experimental_vector_reduce_v2_fadd,        // llvm.experimental.vector.reduce.v2.fadd
        experimental_vector_reduce_v2_fmul,        // llvm.experimental.vector.reduce.v2.fmul
        experimental_vector_reduce_xor,            // llvm.experimental.vector.reduce.xor
        experimental_widenable_condition,          // llvm.experimental.widenable.condition
        fabs,                                      // llvm.fabs
        floor,                                     // llvm.floor
        flt_rounds,                                // llvm.flt.rounds
        fma,                                       // llvm.fma
        fmuladd,                                   // llvm.fmuladd
        frameaddress,                              // llvm.frameaddress
        fshl,                                      // llvm.fshl
        fshr,                                      // llvm.fshr
        gcread,                                    // llvm.gcread
        gcroot,                                    // llvm.gcroot
        gcwrite,                                   // llvm.gcwrite
        get_active_lane_mask,                      // llvm.get.active.lane.mask
        get_dynamic_area_offset,                   // llvm.get.dynamic.area.offset
        hwasan_check_memaccess,                    // llvm.hwasan.check.memaccess
        hwasan_check_memaccess_shortgranules,      // llvm.hwasan.check.memaccess.shortgranules
        icall_branch_funnel,                       // llvm.icall.branch.funnel
        init_trampoline,                           // llvm.init.trampoline
        instrprof_increment,                       // llvm.instrprof.increment
        instrprof_increment_step,                  // llvm.instrprof.increment.step
        instrprof_value_profile,                   // llvm.instrprof.value.profile
        invariant_end,                             // llvm.invariant.end
        invariant_start,                           // llvm.invariant.start
        is_constant,                               // llvm.is.constant
        launder_invariant_group,                   // llvm.launder.invariant.group
        lifetime_end,                              // llvm.lifetime.end
        lifetime_start,                            // llvm.lifetime.start
        llrint,                                    // llvm.llrint
        llround,                                   // llvm.llround
        load_relative,                             // llvm.load.relative
        localaddress,                              // llvm.localaddress
        localescape,                               // llvm.localescape
        localrecover,                              // llvm.localrecover
        log,                                       // llvm.log
        log10,                                     // llvm.log10
        log2,                                      // llvm.log2
        loop_decrement,                            // llvm.loop.decrement
        loop_decrement_reg,                        // llvm.loop.decrement.reg
        lrint,                                     // llvm.lrint
        lround,                                    // llvm.lround
        masked_compressstore,                      // llvm.masked.compressstore
        masked_expandload,                         // llvm.masked.expandload
        masked_gather,                             // llvm.masked.gather
        masked_load,                               // llvm.masked.load
        masked_scatter,                            // llvm.masked.scatter
        masked_store,                              // llvm.masked.store
        matrix_column_major_load,                  // llvm.matrix.column.major.load
        matrix_column_major_store,                 // llvm.matrix.column.major.store
        matrix_multiply,                           // llvm.matrix.multiply
        matrix_transpose,                          // llvm.matrix.transpose
        maximum,                                   // llvm.maximum
        maxnum,                                    // llvm.maxnum
        memcpy,                                    // llvm.memcpy
        memcpy_element_unordered_atomic,           // llvm.memcpy.element.unordered.atomic
        memcpy_inline,                             // llvm.memcpy.inline
        memmove,                                   // llvm.memmove
        memmove_element_unordered_atomic,          // llvm.memmove.element.unordered.atomic
        memset,                                    // llvm.memset
        memset_element_unordered_atomic,           // llvm.memset.element.unordered.atomic
        minimum,                                   // llvm.minimum
        minnum,                                    // llvm.minnum
        nearbyint,                                 // llvm.nearbyint
        objc_arc_annotation_bottomup_bbend,        // llvm.objc.arc.annotation.bottomup.bbend
        objc_arc_annotation_bottomup_bbstart,      // llvm.objc.arc.annotation.bottomup.bbstart
        objc_arc_annotation_topdown_bbend,         // llvm.objc.arc.annotation.topdown.bbend
        objc_arc_annotation_topdown_bbstart,       // llvm.objc.arc.annotation.topdown.bbstart
        objc_autorelease,                          // llvm.objc.autorelease
        objc_autoreleasePoolPop,                   // llvm.objc.autoreleasePoolPop
        objc_autoreleasePoolPush,                  // llvm.objc.autoreleasePoolPush
        objc_autoreleaseReturnValue,               // llvm.objc.autoreleaseReturnValue
        objc_clang_arc_use,                        // llvm.objc.clang.arc.use
        objc_copyWeak,                             // llvm.objc.copyWeak
        objc_destroyWeak,                          // llvm.objc.destroyWeak
        objc_initWeak,                             // llvm.objc.initWeak
        objc_loadWeak,                             // llvm.objc.loadWeak
        objc_loadWeakRetained,                     // llvm.objc.loadWeakRetained
        objc_moveWeak,                             // llvm.objc.moveWeak
        objc_release,                              // llvm.objc.release
        objc_retain,                               // llvm.objc.retain
        objc_retain_autorelease,                   // llvm.objc.retain.autorelease
        objc_retainAutorelease,                    // llvm.objc.retainAutorelease
        objc_retainAutoreleaseReturnValue,         // llvm.objc.retainAutoreleaseReturnValue
        objc_retainAutoreleasedReturnValue,        // llvm.objc.retainAutoreleasedReturnValue
        objc_retainBlock,                          // llvm.objc.retainBlock
        objc_retainedObject,                       // llvm.objc.retainedObject
        objc_storeStrong,                          // llvm.objc.storeStrong
        objc_storeWeak,                            // llvm.objc.storeWeak
        objc_sync_enter,                           // llvm.objc.sync.enter
        objc_sync_exit,                            // llvm.objc.sync.exit
        objc_unretainedObject,                     // llvm.objc.unretainedObject
        objc_unretainedPointer,                    // llvm.objc.unretainedPointer
        objc_unsafeClaimAutoreleasedReturnValue,   // llvm.objc.unsafeClaimAutoreleasedReturnValue
        objectsize,                                // llvm.objectsize
        pcmarker,                                  // llvm.pcmarker
        pow,                                       // llvm.pow
        powi,                                      // llvm.powi
        prefetch,                                  // llvm.prefetch
        preserve_array_access_index,               // llvm.preserve.array.access.index
        preserve_struct_access_index,              // llvm.preserve.struct.access.index
        preserve_union_access_index,               // llvm.preserve.union.access.index
        ptr_annotation,                            // llvm.ptr.annotation
        ptrmask,                                   // llvm.ptrmask
        read_register,                             // llvm.read_register
        read_volatile_register,                    // llvm.read_volatile_register
        readcyclecounter,                          // llvm.readcyclecounter
        returnaddress,                             // llvm.returnaddress
        rint,                                      // llvm.rint
        round,                                     // llvm.round
        roundeven,                                 // llvm.roundeven
        sadd_sat,                                  // llvm.sadd.sat
        sadd_with_overflow,                        // llvm.sadd.with.overflow
        sdiv_fix,                                  // llvm.sdiv.fix
        sdiv_fix_sat,                              // llvm.sdiv.fix.sat
        set_loop_iterations,                       // llvm.set.loop.iterations
        sideeffect,                                // llvm.sideeffect
        sin,                                       // llvm.sin
        smul_fix,                                  // llvm.smul.fix
        smul_fix_sat,                              // llvm.smul.fix.sat
        smul_with_overflow,                        // llvm.smul.with.overflow
        sponentry,                                 // llvm.sponentry
        sqrt,                                      // llvm.sqrt
        ssa_copy,                                  // llvm.ssa.copy
        ssub_sat,                                  // llvm.ssub.sat
        ssub_with_overflow,                        // llvm.ssub.with.overflow
        stackguard,                                // llvm.stackguard
        stackprotector,                            // llvm.stackprotector
        stackrestore,                              // llvm.stackrestore
        stacksave,                                 // llvm.stacksave
        strip_invariant_group,                     // llvm.strip.invariant.group
        test_set_loop_iterations,                  // llvm.test.set.loop.iterations
        thread_pointer,                            // llvm.thread.pointer
        trap,                                      // llvm.trap
        trunc,                                     // llvm.trunc
        type_checked_load,                         // llvm.type.checked.load
        type_test,                                 // llvm.type.test
        uadd_sat,                                  // llvm.uadd.sat
        uadd_with_overflow,                        // llvm.uadd.with.overflow
        udiv_fix,                                  // llvm.udiv.fix
        udiv_fix_sat,                              // llvm.udiv.fix.sat
        umul_fix,                                  // llvm.umul.fix
        umul_fix_sat,                              // llvm.umul.fix.sat
        umul_with_overflow,                        // llvm.umul.with.overflow
        usub_sat,                                  // llvm.usub.sat
        usub_with_overflow,                        // llvm.usub.with.overflow
        vacopy,                                    // llvm.va_copy
        vaend,                                     // llvm.va_end
        vastart,                                   // llvm.va_start
        var_annotation,                            // llvm.var.annotation
        vp_add,                                    // llvm.vp.add
        vp_and,                                    // llvm.vp.and
        vp_ashr,                                   // llvm.vp.ashr
        vp_lshr,                                   // llvm.vp.lshr
        vp_mul,                                    // llvm.vp.mul
        vp_or,                                     // llvm.vp.or
        vp_sdiv,                                   // llvm.vp.sdiv
        vp_shl,                                    // llvm.vp.shl
        vp_srem,                                   // llvm.vp.srem
        vp_sub,                                    // llvm.vp.sub
        vp_udiv,                                   // llvm.vp.udiv
        vp_urem,                                   // llvm.vp.urem
        vp_xor,                                    // llvm.vp.xor
        vscale,                                    // llvm.vscale
        write_register,                            // llvm.write_register
        xray_customevent,                          // llvm.xray.customevent
        xray_typedevent,                           // llvm.xray.typedevent
        num_intrinsics = 8052
    };

    struct SlotTracker;


    static_assert(sizeof(Value) <= 2 * sizeof(u64));

    struct SlotTracker
    {
        u64 next_id;
        u64 not_used;
        u64 starting_id;
        Buffer<Value*> map;

        static SlotTracker create(Allocator* allocator, s64 count)
        {
            SlotTracker slot_tracker = {
                .map = slot_tracker.map.create(allocator, count),
            };
            
            return slot_tracker;
        }

        u64 find(Value* value)
        {
            for (auto i = 0; i < map.len; i++)
            {
                if (map[i] == value)
                {
                    return i + starting_id;
                }
            }

            return UINT64_MAX;
        }

        template <typename T>
        u64 new_id(T value)
        {
            auto id = next_id++;
            map.append(reinterpret_cast<Value*>(value));
            return id;
        }
    };

    struct Argument
    {
        Value value;
        s64 arg_index;
    };


    struct ConstantInt
    {
        Value e;
        union
        {
            u64 eight_byte;
            u64* more_than_eight_byte;
        } value;
        u32 bit_count;
        bool is_signed;

        static ConstantInt* get(Allocator* allocator, RNS::Type* type, u64 value, bool is_signed)
        {
            assert(type);
            assert(type->id == TypeID::Integer);
            auto* integer_type = reinterpret_cast<IntegerType*>(type);
            auto bits = integer_type->bits;
            assert(bits >= 1 && bits <= 64);

            auto* new_int = new(allocator) ConstantInt;
            assert(new_int);
            *new_int = {
                .e = {
                    .type = type,
                    .base_id = ValueID::ConstantInt,
                },
                .value = value,
                .bit_count = bits,
                .is_signed = is_signed,
            };

            return new_int;
        }
    };

    enum class CmpType : u8
    {
        // Opcode            U L G E    Intuitive operation
        FCMP_FALSE = 0, ///< 0 0 0 0    Always false (always folded)
        FCMP_OEQ = 1,   ///< 0 0 0 1    True if ordered and equal
        FCMP_OGT = 2,   ///< 0 0 1 0    True if ordered and greater than
        FCMP_OGE = 3,   ///< 0 0 1 1    True if ordered and greater than or equal
        FCMP_OLT = 4,   ///< 0 1 0 0    True if ordered and less than
        FCMP_OLE = 5,   ///< 0 1 0 1    True if ordered and less than or equal
        FCMP_ONE = 6,   ///< 0 1 1 0    True if ordered and operands are unequal
        FCMP_ORD = 7,   ///< 0 1 1 1    True if ordered (no nans)
        FCMP_UNO = 8,   ///< 1 0 0 0    True if unordered: isnan(X) | isnan(Y)
        FCMP_UEQ = 9,   ///< 1 0 0 1    True if unordered or equal
        FCMP_UGT = 10,  ///< 1 0 1 0    True if unordered or greater than
        FCMP_UGE = 11,  ///< 1 0 1 1    True if unordered, greater than, or equal
        FCMP_ULT = 12,  ///< 1 1 0 0    True if unordered or less than
        FCMP_ULE = 13,  ///< 1 1 0 1    True if unordered, less than, or equal
        FCMP_UNE = 14,  ///< 1 1 1 0    True if unordered or not equal
        FCMP_TRUE = 15, ///< 1 1 1 1    Always true (always folded)
        FIRST_FCMP_PREDICATE = FCMP_FALSE,
        LAST_FCMP_PREDICATE = FCMP_TRUE,
        BAD_FCMP_PREDICATE = FCMP_TRUE + 1,
        ICMP_EQ = 32,  ///< equal
        ICMP_NE = 33,  ///< not equal
        ICMP_UGT = 34, ///< unsigned greater than
        ICMP_UGE = 35, ///< unsigned greater or equal
        ICMP_ULT = 36, ///< unsigned less than
        ICMP_ULE = 37, ///< unsigned less or equal
        ICMP_SGT = 38, ///< signed greater than
        ICMP_SGE = 39, ///< signed greater or equal
        ICMP_SLT = 40, ///< signed less than
        ICMP_SLE = 41, ///< signed less or equal
        FIRST_ICMP_PREDICATE = ICMP_EQ,
        LAST_ICMP_PREDICATE = ICMP_SLE,
        BAD_ICMP_PREDICATE = ICMP_SLE + 1
    };

    struct CallBase
    {
        FunctionType* function_type;
    };

    struct AllocaInstruction
    {
        Type* allocated_type;
    };

    struct GetElementPtrInstruction
    {
        Type* source_type;
        Type* result_type;
    };

    struct Compare
    {
        CmpType type;

        const char* to_string()
        {
            switch (type)
            {
                case CmpType::ICMP_EQ:
                    return "eq";
                case CmpType::ICMP_NE:
                    return "ne";
                case CmpType::ICMP_SLT:
                    return "slt";
                case CmpType::ICMP_SGT:
                    return "sgt";
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }
            return nullptr;
        }
    };

    const char* type_to_string(Type* type, char* buffer)
    {
        assert(type);
        switch (type->id)
        {
            case TypeID::Pointer:
            {
                char aux_buffer[64];
                auto* pointer_type = reinterpret_cast<PointerType*>(type);
                sprintf(buffer, "%s*", type_to_string(pointer_type->type, aux_buffer));
            } break;
            case TypeID::Integer:
            {
                auto* integer_type = reinterpret_cast<IntegerType*>(type);
                auto bits = integer_type->bits;
                sprintf(buffer, "i%u", bits);
            } break;
            case TypeID::Void:
            {
                sprintf(buffer, "void");
            } break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }
        return (const char*)buffer;
    }

    struct Module
    {
        Buffer<Function> functions;

        static Module create(Allocator* allocator)
        {
            Module module = {
                .functions = module.functions.create(allocator, 64),
            };

            return module;
        }

        // @TODO: do typechecking
        Function* find_function(StringView name, Type* type = nullptr);
    };

    struct Function
    {
        // @Info: for a function, a value type is the returning type
        Value value;
        StringView name;
        Type* type;
        Buffer<BasicBlock*> basic_blocks;
        Slice<Argument> arguments;
        Module* parent;
        // @TODO: symbol table

        static Function* create(Module* module, Type* type, /* @TODO: linkage*/ String name)
        {
            assert(type);
            auto* function_type = reinterpret_cast<FunctionType*>(type);
            auto* ret_type = function_type->ret_type;
            assert(ret_type);

            auto* function = module->functions.allocate();
            *function = {
                .value = {
                    .type = ret_type,
                    .base_id = ValueID::GlobalFunction,
                 },
                .name = StringView::create(name.ptr, name.len),
                .type = type,
                .parent = module,
                // @TODO: basic blocks, args...
            };

            return function;
        }

        static FunctionType* get_type(Type* return_type, Slice<Type*> types, bool var_args)
        {
            RNS_NOT_IMPLEMENTED;
            return nullptr;
        }

        void print(Allocator* allocator);
    };

    struct Instruction
    {
        struct
        {
            Value value;
            InstructionID id;
            BasicBlock* parent;
        } base;

        Value* operands[15];
        u8 operand_count;
        u64 id1;
        u64 id2;
        u64 id3;

        union
        {
            AllocaInstruction alloca_i;
            GetElementPtrInstruction get_element_ptr;
            Compare compare;
        };

        void print(SlotTracker& slot_tracker)
        {
            char operand0[64] = {};
            char operand1[64] = {};
            char operand2[64] = {};
            char type_buffer[64];

            printf("\t");
            switch (base.id)
            {
                case InstructionID::Alloca:
                {
                    auto* pointer_type = this->base.value.type;
                    assert(pointer_type);
                    auto* pointer_type_cast = reinterpret_cast<PointerType*>(pointer_type);
                    printf("%%%llu = alloca %s, align 4", id1, type_to_string(pointer_type_cast->type, type_buffer));
                } break;
                case InstructionID::Store:
                {
                    printf("store i32 %s, i32* %s, align 4", operands[0]->print(operand0, slot_tracker), operands[1]->print(operand1, slot_tracker));
                } break;
                case InstructionID::Br:
                {
                    if (operand_count == 1)
                    {
                        printf("br label %s", operands[0]->print(operand0, slot_tracker));
                    }
                    else
                    {
                        printf("br i1 %s, label %s, label %s", operands[2]->print(operand2, slot_tracker), operands[0]->print(operand0, slot_tracker), operands[1]->print(operand1, slot_tracker));
                    }
                } break;
                case InstructionID::Load:
                {
                    printf("%%%llu = load i32, i32* %s, align 4", id3, operands[0]->print(operand0, slot_tracker));
                } break;
                case InstructionID::ICmp:
                {
                    printf("%%%llu = icmp %s i32 %s, %s", id3, compare.to_string(), operands[0]->print(operand0, slot_tracker), operands[1]->print(operand1, slot_tracker));
                } break;
                case InstructionID::Add:
                {
                    printf("%%%llu = add i32 %s, %s, align 4", id3, operands[0]->print(operand0, slot_tracker), operands[1]->print(operand1, slot_tracker));
                } break;
                case InstructionID::Sub:
                {
                    printf("%%%llu = sub i32 %s, %s, align 4", id3, operands[0]->print(operand0, slot_tracker), operands[1]->print(operand1, slot_tracker));
                } break;
                case InstructionID::Mul:
                {
                    printf("%%%llu = mul i32 %s, %s, align 4", id3, operands[0]->print(operand0, slot_tracker), operands[1]->print(operand1, slot_tracker));
                } break;
                case InstructionID::Ret:
                {
                    if (operands[0])
                    {
                        printf("ret i32 %s", operands[0]->print(operand0, slot_tracker));
                    }
                    else
                    {
                        printf("ret void");
                    }
                } break;
                case InstructionID::Call:
                {
                    Value* callee = operands[0];
                    assert(callee->base_id == ValueID::GlobalFunction);
                    auto* ret_type = callee->type;
                    assert(ret_type);
                    Function* callee_function = reinterpret_cast<Function*>(callee);
                    assert(callee_function);
                    auto ret_type_not_void = ret_type->id != TypeID::Void;
                    if (ret_type_not_void)
                    {
                        printf("%%%llu = ", id1);
                    }
                    printf("call i32 @%s(", callee_function->name.get());

                    auto arg_count = operand_count - 1;
                    if (arg_count)
                    {
                        auto print_arg = [](Value* operand)
                        {
#if 0
                            char type_buffer[64];

                            if (operand->typeID == TypeID::LabelType)
                            {
                                // @MaybeBuggy Here are interpreting this operand is a load, which might be not true
                                auto* instr = reinterpret_cast<Instruction*>(operand);
                                assert(instr);
                                switch (instr->base.id)
                                {
                                    case InstructionID::Alloca:
                                    {
                                        printf("%s %%%llu", type_to_string(operand->type, type_buffer), reinterpret_cast<Instruction*>(operand)->id1);
                                    } break;
                                    case InstructionID::Load:
                                    {
                                        printf("%s %%%llu", type_to_string(operand->type, type_buffer), reinterpret_cast<Instruction*>(operand)->id3);
                                    } break;
                                    default:
                                        RNS_NOT_IMPLEMENTED;
                                        break;
                                }
                            }
                            else
                            {
                                RNS_NOT_IMPLEMENTED;
                            }
#endif
                        };

                        for (auto i = 1; i < arg_count; i++)
                        {
                            auto* operand = operands[i];
                            print_arg(operand);
                            printf(", ");
                        }
                        auto* operand = operands[arg_count];
                        print_arg(operand);
                    }

                    printf(")");
                } break;
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }

            printf("\n");
        }

        void get_info(SlotTracker& slot_tracker)
        {
            switch (base.id)
            {
                case InstructionID::Alloca:
                {
                    id1 = slot_tracker.new_id(this);
                } break;
                case InstructionID::Load:
                {
                    id3 = slot_tracker.new_id(this);
                } break;
                case InstructionID::ICmp:
                {
                    id3 = slot_tracker.new_id(this);
                } break;
                case InstructionID::Mul: case InstructionID::Add: case InstructionID::Sub:
                {
                    id3 = slot_tracker.new_id(this);
                } break;
                case InstructionID::Call:
                {
                    id1 = slot_tracker.new_id(this);
                } break;
                case InstructionID::Store:
                case InstructionID::Br:
                case InstructionID::Ret:
                {
                } break;
                default:
                    RNS_NOT_IMPLEMENTED;
                    break;
            }
        }
    };

    const char* Value::print(char* buffer, SlotTracker& slot_tracker)
    {
        // @Info: we shouldn't be null here. Void values are treated in the return printing case.
        assert(this);
        switch (this->base_id)
        {
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }

        return (const char*)buffer;
    }



    inline Function* Module::find_function(StringView name, Type* type)
    {
        assert(functions.len);
        for (auto& function : functions)
        {
            if (name.equal(function.name))
            {
                return &function;
            }
        }

        return nullptr;
    }

    struct BasicBlock
    {
        Value value;
        Function* parent;
        Buffer<Instruction*> instructions;
        u32 use_count;

        u64 id;

        static BasicBlock create(Context& context, String name = {})
        {
            // @TODO: consider dealing with inserting the block in the middle of the array
            BasicBlock new_basic_block = {
                .value = {
                    .type = context.get_label_type(),
                    .base_id = ValueID::BasicBlock,
                }
            };

            return new_basic_block;
        }
        void get_id(SlotTracker& slot_tracker)
        {
            if (parent->basic_blocks[0] != this)
            {
                id = slot_tracker.new_id(this);
            }
        }

        void print(SlotTracker& slot_tracker)
        {
            // @Info: Don't print function's main scope
            if (parent->basic_blocks[0] != this)
            {
                printf("%llu:\n", id);
            }
        }
    };

    void Function::print(Allocator* allocator)
    {
        SlotTracker slot_tracker = SlotTracker::create(allocator, 2048);
        // @TODO: change hardcoding
        assert(slot_tracker.starting_id == 0);
        assert(slot_tracker.next_id == 0);

        for (auto& arg: arguments)
        {
            slot_tracker.new_id(&arg);
        }
        slot_tracker.new_id(nullptr);

        for (auto* block : basic_blocks)
        {
            block->get_id(slot_tracker);
            for (auto* instruction : block->instructions)
            {
                instruction->get_info(slot_tracker);
            }
        }

        for (auto* block : basic_blocks)
        {
            for (auto* instruction : block->instructions)
            {
                if (instruction->base.id == InstructionID::Br)
                {
                    instruction->get_info(slot_tracker);
                }
            }
        }

        char ret_type_buffer[64];
        auto* type = this->value.type;
        assert(type);
        auto* function_type = reinterpret_cast<FunctionType*>(type);
        auto* ret_type = function_type->ret_type;
        assert(ret_type);
        printf("\ndefine dso_local %s @%s(", type_to_string(ret_type, ret_type_buffer), name.get());

        // Argument printing
        if (arguments.len)
        {
            auto buffer_len = 0;
            auto last_index = arguments.len - 1;
            char type_buffer[64];
            for (auto i = 0; i < last_index; i++)
            {
                auto* type = arguments[i].value.type;
                assert(type);
                printf("%s %%%lld, ", type_to_string(type, type_buffer), arguments[i].arg_index);
            }
            auto* type = arguments[last_index].value.type;
            assert(type);
            printf("%s %%%lld", type_to_string(type, type_buffer), arguments[last_index].arg_index);
        }
        printf(")\n{\n");

        // @Info: actual printing
        for (auto* block : basic_blocks)
        {
            block->print(slot_tracker);
            for (auto* instruction : block->instructions)
            {
                instruction->print(slot_tracker);
            }
        }
        printf("}\n");
    }

    struct Builder
    {
        Context& context;
        BasicBlock* current;
        Function* function;
        /// <summary> Guarantee pointer stability for members
        BasicBlockBuffer* basic_block_buffer;
        InstructionBuffer* instruction_buffer;
        Module* module;
        /// </summary>
        s64 next_alloca_index;
        // @TODO: remove from here? Should this be userspace?
        /**/
        Instruction* return_alloca;
        BasicBlock* exit_block;
        bool conditional_alloca;
        // @TODO: this should be taken into account in every branch statement
        bool emitted_return;
        bool explicit_return;
        /**/

        // @TODO: guarantee pointer stability
        Instruction* create_alloca(Type* type, Value* array_size = nullptr, const char* name = nullptr)
        {
            Instruction i = {
                .base {
                    .value = {
                        .type = context.get_pointer_type(type),
                        .base_id = ValueID::Instruction,
                    },
                    .id = InstructionID::Alloca,
                },
                .alloca_i = {
                    .allocated_type = type,
                }
            };

            return insert_alloca(i);
        }

        Instruction* create_store(Value* value, Value* ptr, bool is_volatile = false)
        {
            Instruction i = {
                .base = {
                    .value = {
                        .type = context.get_void_type(),
                        .base_id = ValueID::Instruction,
                    },
                    .id = InstructionID::Store,
                },
                .operands = { value, ptr },
                .operand_count = 2,
            };

            return insert_at_end(i);
        }

        Instruction* create_load(Type* type, Value* value, bool is_volatile = false, String name = {})
        {
            Instruction i = {
                .base = {
                    .value = {
                        .type = type,
                        .base_id = ValueID::Instruction,
                    },
                    .id = InstructionID::Load,
                },
                .operands = { value},
                .operand_count = 1,
            };

            return insert_at_end(i);
        }

        void append_to_function(BasicBlock* block)
        {
            assert(function);

            function->basic_blocks.append(block);
            block->parent = function;
        }

        BasicBlock* set_block(BasicBlock* block)
        {
            assert(current != block);
            auto* previous = current;
            current = block;
            return previous;
        }

        BasicBlock* create_block(Allocator* allocator, String name = {})
        {
            BasicBlock* basic_block = basic_block_buffer->append(BasicBlock::create(context, name));
            // @TODO: this should change
            basic_block->instructions = basic_block->instructions.create(allocator, 64);
            return basic_block;
        }

        Instruction* create_br(BasicBlock* dst_block)
        {
            assert(dst_block);
            if (!is_terminated())
            {
                Instruction i = {
                    .base = {
                        .value = {
                            .type = context.get_void_type(),
                            .base_id = ValueID::Instruction,
                        },
                        .id = InstructionID::Br,
                    },
                    .operands = { reinterpret_cast<Value*>(dst_block) },
                    .operand_count = 1,
                };

                dst_block->use_count++;
                return insert_at_end(i);
            }
            return nullptr;
        }

        Instruction* create_conditional_br(BasicBlock* true_block, BasicBlock* else_block, Value* condition)
        {
            if (!is_terminated())
            {
                Instruction i = {
                    .base = {
                        .value = {
                            .type = context.get_void_type(),
                            .base_id = ValueID::Instruction,
                        },
                        .id = InstructionID::Br,
                    },
                    .operands = { reinterpret_cast<Value*>(true_block), reinterpret_cast<Value*>(else_block), condition },
                    .operand_count = 3,
                };
                true_block->use_count++;
                else_block->use_count++;

                return insert_at_end(i);
            }
            return nullptr;
        }

        Instruction* create_icmp(CmpType compare_type, Value* left, Value* right, const char* name = nullptr)
        {
            Instruction i = {
                .base = {
                    // @TODO: we need to switch to a vector comparison for SIMD types
                    .value = {
                        .type = context.get_boolean_type(),
                        .base_id = ValueID::Instruction,
                    },
                    .id = InstructionID::ICmp,
                },
                .operands = { left, right },
                .operand_count = 2,
                .compare = { .type = compare_type }
            };

            return insert_at_end(i);
        }

        Instruction* create_add(Value* left, Value* right, const char* name = nullptr)
        {
            assert(left->type == right->type);
            Instruction i = {
                .base = {
                    .value = {
                        .type = left->type,
                        .base_id = ValueID::Instruction,
                    },
                    .id = InstructionID::Add,
                },
                .operands = { left, right },
                .operand_count = 2,
            };

            return insert_at_end(i);
        }

        Instruction* create_sub(Value* left, Value* right, const char* name = nullptr)
        {
            assert(left->type == right->type);
            Instruction i = {
                .base = {
                    .value = {
                        .type = left->type,
                        .base_id = ValueID::Instruction,
                    },
                    .id = InstructionID::Sub,
                },
                .operands = { left, right },
                .operand_count = 2,
            };

            return insert_at_end(i);
        }

        Instruction* create_mul(Value* left, Value* right, const char* name = nullptr)
        {
            assert(left->type == right->type);
            Instruction i = {
                .base = {
                    .value = {
                        .type = left->type,
                        .base_id = ValueID::Instruction,
                    },
                    .id = InstructionID::Mul,
                },
                .operands = { left, right },
                .operand_count = 2,
            };

            return insert_at_end(i);
        }

        Instruction* create_ret(Value* value)
        {
            auto* fn_type_base = function->type;
            assert(fn_type_base);
            assert(fn_type_base->id == TypeID::Function);
            auto* function_type = reinterpret_cast<FunctionType*>(fn_type_base);

            auto* function_ret_type = function_type->ret_type;
            assert(function_ret_type);
            assert(value->type);
            assert(value->type == function_ret_type);
            if (!is_terminated())
            {
                Instruction i = {
                    .base = {
                        .value = {
                            .type = value->type,
                            .base_id = ValueID::Instruction,
                        },
                        .id = InstructionID::Ret,
                    },
                    .operands = { value },
                    .operand_count = 1,
                };

                return insert_at_end(i);
            }
            return nullptr;
        }

        Instruction* create_ret_void()
        {
            return create_ret(nullptr);
        }

        Instruction* create_call(Value* callee, Slice<Value*> arguments = {})
        {
            Instruction i = {
                .base = {
                    .value = {
                        .type = callee->type,
                        .base_id = ValueID::Instruction,
                    },
                    .id = InstructionID::Call,
                },
            };

            assert(arguments.len <= rns_array_length(i.operands) - 1);

            i.operand_count = arguments.len + 1;
            i.operands[0] = callee;

            for (auto index = 0; index < arguments.len; index++)
            {
                i.operands[index + 1] = arguments[index];
            }

            return insert_at_end(i);
        }

        Instruction* create_inbounds_GEP(Type* type, Value* pointer, Slice<Value*> indices)
        {
            Instruction i = {
                .base = {
                    .value = {
                        .type = type,
                        .base_id = ValueID::Instruction,
                    },
                    .id = InstructionID::GetElementPtr,
                },
            };

            assert(indices.len == 2);
            i.operands[0] = pointer;

            int it = 1;
            for (auto& index_value : indices)
            {
                i.operands[it++] = index_value;
            }
            i.operand_count = it;

            return insert_at_end(i);
        }
    private:
        Instruction* insert_alloca(Instruction alloca_instruction)
        {
            assert(function);
            assert(instruction_buffer);

            auto* i = instruction_buffer->append(alloca_instruction);
            auto* entry_block = function->basic_blocks[0];
            entry_block->instructions.insert_at(i, next_alloca_index++);
            i->base.parent = entry_block;
            return i;
        }

        inline bool is_terminated()
        {
            assert(current);
            assert(current->instructions.cap);
            if (current->instructions.len)
            {
                auto& last_instruction = current->instructions[current->instructions.len - 1];
                switch (last_instruction->base.id)
                {
                    case InstructionID::Br: case InstructionID::Ret:
                        return true;
                    default:
                        return false;
                }
            }
            else
            {
                return false;
            }
        }

        inline Instruction* insert_at_end(Instruction instruction)
        {
            assert(function);
            assert(instruction_buffer);
            assert(current);
            auto* i = instruction_buffer->append(instruction);
            auto* result = current->instructions.append(i);
            i->base.parent = current;
            return i;
        }
    };

    bool introspect_for_conditional_allocas(Node* scope)
    {
        if (!scope)
        {
            return false;
        }

        for (auto* st_node : scope->block.statements)
        {
            if (st_node->type == NodeType::Ret)
            {
                return true;
            }
            else if (st_node->type == NodeType::Conditional)
            {
                if (introspect_for_conditional_allocas(st_node->conditional.if_block))
                {
                    return true;
                }
                if (introspect_for_conditional_allocas(st_node->conditional.else_block))
                {
                    return true;
                }
            }
            else if (st_node->type == NodeType::Loop)
            {
                if (introspect_for_conditional_allocas(st_node->loop.body))
                {
                    return true;
                }
            }
        }

        return false;
    }

    void emit_jump(Allocator* allocator, Builder& builder, BasicBlock* dst_block)
    {
        builder.create_br(dst_block);
        auto* new_block = builder.create_block(allocator);
        builder.append_to_function(new_block);
        builder.set_block(new_block);
    }

    Value* do_node(Allocator* allocator, Builder& builder, Node* node, Type* expected_type = nullptr)
    {
        switch (node->type)
        {
            case NodeType::Block:
            {
                for (auto* st_node : node->block.statements)
                {
                    if (!builder.emitted_return)
                    {
                        do_node(allocator, builder, st_node);
                    }
                }
            } break;
            case NodeType::Conditional:
            {
                bool saved_emitted_return = builder.emitted_return;
                auto* ast_condition = node->conditional.condition;
                auto* ast_if_block = node->conditional.if_block;
                auto* ast_else_block = node->conditional.else_block;

                auto* exit_block = builder.create_block(allocator);
                auto* if_block = builder.create_block(allocator);
                BasicBlock* else_block = exit_block;
                if (ast_else_block)
                {
                    else_block = builder.create_block(allocator);
                }

                node->conditional.exit_block = exit_block;

                auto* condition = do_node(allocator, builder, ast_condition, builder.context.get_boolean_type());
                assert(condition);

                auto exit_block_in_use = true;
                if (if_block != else_block)
                {
                    builder.create_conditional_br(if_block, else_block, condition);
                }
                else
                {
                    if ((exit_block_in_use = exit_block->use_count))
                    {
                        builder.create_br(exit_block);
                    }
                }

                builder.emitted_return = false;
                builder.append_to_function(if_block);
                builder.set_block(if_block);
                do_node(allocator, builder, ast_if_block);
                bool if_block_returned = builder.emitted_return;

                builder.create_br(exit_block);

                builder.emitted_return = false;
                if (else_block != exit_block)
                {
                    builder.append_to_function(else_block);
                    builder.set_block(else_block);
                    do_node(allocator, builder, ast_else_block);

                    builder.create_br(exit_block);
                }
                bool else_block_returned = builder.emitted_return;

                builder.emitted_return = if_block_returned && else_block_returned;

                if (exit_block_in_use && !builder.emitted_return)
                {
                    builder.append_to_function(exit_block);
                    builder.set_block(exit_block);
                }
            } break;
            case NodeType::Loop:
            {
                auto* ast_loop_prefix = node->loop.prefix;
                auto* ast_loop_body = node->loop.body;
                auto* ast_loop_postfix = node->loop.postfix;

                auto loop_prefix_block = builder.create_block(allocator);
                auto loop_body_block = builder.create_block(allocator);
                auto loop_postfix_block = builder.create_block(allocator);
                auto loop_end_block = builder.create_block(allocator);

                auto& loop_continue_block = loop_postfix_block;
                node->loop.continue_block = loop_continue_block;
                node->loop.exit_block = loop_end_block;

                builder.create_br(loop_prefix_block);
                builder.append_to_function(loop_prefix_block);
                builder.set_block(loop_prefix_block);

                assert(ast_loop_prefix->block.statements.len == 1);
                auto* ast_condition = ast_loop_prefix->block.statements[0];
                auto* condition = do_node(allocator, builder, ast_condition, builder.context.get_boolean_type());
                assert(condition);

                builder.create_conditional_br(loop_body_block, loop_end_block, condition);
                builder.append_to_function(loop_body_block);
                builder.set_block(loop_body_block);

                do_node(allocator, builder, ast_loop_body);

                builder.create_br(loop_postfix_block);

                builder.append_to_function(loop_postfix_block);
                builder.set_block(loop_postfix_block);
                do_node(allocator, builder, ast_loop_postfix);

                if (!builder.emitted_return)
                {
                    builder.create_br(loop_prefix_block);
                    builder.append_to_function(loop_end_block);
                    builder.set_block(loop_end_block);
                }
            } break;
            case NodeType::Break:
            {
                auto* ast_jump_target = node->break_.target;
                assert(ast_jump_target->type == NodeType::Loop);
                auto* jump_target = reinterpret_cast<BasicBlock*>(ast_jump_target->loop.exit_block);
                assert(jump_target);

                // @TODO: this may be buggy
#if 0
                emit_jump(allocator, builder, jump_target);
#else
                builder.create_br(jump_target);
#endif
            } break;
            case NodeType::VarDecl:
            {
                auto* type = node->var_decl.type;
                assert(type);
                auto* rns_type = RNS::get_type(allocator, builder.context, type);
                assert(rns_type);
                auto* var_alloca = builder.create_alloca(rns_type);
                node->var_decl.backend_ref = var_alloca;

                auto* value = node->var_decl.value;
                if (value)
                {
                    switch (rns_type->id)
                    {
                        case TypeID::Array:
                        {
                            auto* expression = do_node(allocator, builder, value, rns_type);
                            assert(expression);
                            assert(expression->base_id == ValueID::ConstantArray);
                            // call intrinsic memcpy to copy the array constant to the array alloca
                            RNS_NOT_IMPLEMENTED;
                        } break;
                        default:
                        {
                            auto* expression = do_node(allocator, builder, value, rns_type);
                            assert(expression);
                            builder.create_store(expression, reinterpret_cast<Value*>(var_alloca), false);
                        } break;
                    }
                }
                else
                {
                    RNS_NOT_IMPLEMENTED;
                }
            } break;
            case NodeType::IntLit:
            {
                auto* result = ConstantInt::get(allocator, builder.context.get_integer_type(32), node->int_lit.lit, node->int_lit.is_signed);
                assert(result);
                return reinterpret_cast<Value*>(result);
            }
            case NodeType::BinOp:
            {
                auto* ast_left = node->bin_op.left;
                auto* ast_right = node->bin_op.right;
                auto binary_op_type = node->bin_op.op;
                assert(ast_left);
                assert(ast_right);

                if (binary_op_type == BinOp::Assign)
                {
                    switch (ast_left->type)
                    {
                        case NodeType::VarExpr:
                        {
                            auto* var_decl = ast_left->var_expr.mentioned;
                            assert(var_decl);
                            auto* alloca_value = var_decl->var_decl.backend_ref;
                            auto* var_type = var_decl->var_decl.type;
                            assert(var_type);
                            auto* rns_var_type = get_type(allocator, builder.context, var_type);
                            assert(rns_var_type);
                            auto* right_value = do_node(allocator, builder, ast_right, rns_var_type);
                            assert(right_value);
                            builder.create_store(right_value, reinterpret_cast<Value*>(alloca_value));
                        } break;
                        case NodeType::UnaryOp:
                        {
                            assert(ast_left->unary_op.type == UnaryOp::PointerDereference);

                            auto* right_value = do_node(allocator, builder, ast_right);
                            assert(right_value);

                            auto* pointer_load = do_node(allocator, builder, ast_left);
                            builder.create_store(right_value, pointer_load);
                        } break;
                        default:
                            RNS_UNREACHABLE;
                    }
                }
                else
                {
                    auto* left = do_node(allocator, builder, ast_left);
                    auto* right = do_node(allocator, builder, ast_right);
                    assert(left);
                    assert(right);

                    Instruction* binary_op_instruction;
                    switch (binary_op_type)
                    {
                        case BinOp::Cmp_LessThan:
                        {
                            binary_op_instruction = builder.create_icmp(CmpType::ICMP_SLT, left, right);
                        } break;
                        case BinOp::Cmp_GreaterThan:
                        {
                            binary_op_instruction = builder.create_icmp(CmpType::ICMP_SGT, left, right);
                        } break;
                        case BinOp::Cmp_Equal:
                        {
                            binary_op_instruction = builder.create_icmp(CmpType::ICMP_EQ, left, right);
                        } break;
                        case BinOp::Plus:
                        {
                            binary_op_instruction = builder.create_add(left, right);
                        } break;
                        case BinOp::Minus:
                        {
                            binary_op_instruction = builder.create_sub(left, right);
                        } break;
                        case BinOp::Mul:
                        {
                            binary_op_instruction = builder.create_mul(left, right);
                        } break;
                        default:
                            RNS_NOT_IMPLEMENTED;
                            break;
                    }

                    return reinterpret_cast<Value*>(binary_op_instruction);
                }
            } break;
            case NodeType::VarExpr:
            {
                auto* var_decl = node->var_expr.mentioned;
                assert(var_decl);
                auto* alloca_ptr = var_decl->var_decl.backend_ref;
                assert(alloca_ptr);
                Instruction* var_alloca = reinterpret_cast<Instruction*>(alloca_ptr);

                auto* type = var_decl->var_decl.type;
                assert(type);
                auto* rns_type = get_type(allocator, builder.context, type);
                assert(rns_type);
                return reinterpret_cast<Value*>(builder.create_load(rns_type, reinterpret_cast<Value*>(var_alloca)));
            } break;
            case NodeType::Ret:
            {
                // @TODO: tolerate this in the future?
                assert(!builder.emitted_return);
                auto* ast_return_expression = node->ret.expr;
                if (ast_return_expression)
                {
                    builder.emitted_return = true;
                    builder.explicit_return = true;

                    assert(ast_return_expression);
                    auto* ret_value = do_node(allocator, builder, ast_return_expression);

                    if (builder.conditional_alloca)
                    {
                        assert(builder.return_alloca);
                        assert(builder.exit_block);
                        builder.create_store(ret_value, reinterpret_cast<Value*>(builder.return_alloca));
                        builder.create_br(builder.exit_block);
                    }
                    else
                    {
                        builder.create_ret(ret_value);
                    }
                }
                else
                {
                    if (!builder.explicit_return)
                    {
                        builder.create_ret_void();
                    }
                    else
                    {
                        builder.create_br(builder.exit_block);
                        builder.emitted_return = true;
                        builder.explicit_return = true;
                    }
                }
            } break;
            case NodeType::InvokeExpr:
            {
                auto* invoke_expr = node->invoke_expr.expr;
                assert(invoke_expr);
                assert(invoke_expr->type == NodeType::Function);
                auto function_name = invoke_expr->function.name;
                // @TODO: do typechecking
                auto* function = builder.module->find_function(StringView::create(function_name.ptr, function_name.len));
                assert(function);

                auto arg_count = node->invoke_expr.arguments.len;
                if (arg_count == 0)
                {
                    auto* call = builder.create_call(reinterpret_cast<Value*>(function));
                    return reinterpret_cast<Value*>(call);
                }
                else
                {
                    Value* argument_buffer[255];
                    assert(arg_count <= 15);
                    auto& node_arg_buffer = node->invoke_expr.arguments;
                    auto arg_i = 0;
                    auto* fn_type_base = function->value.type;
                    assert(fn_type_base);
                    auto* function_type = reinterpret_cast<FunctionType*>(fn_type_base);
                    for (auto* arg_node : node_arg_buffer)
                    {
                        auto* arg = do_node(allocator, builder, arg_node);
                        assert(arg);
                        // @TODO: this may be buggy
                        arg->type = function_type->arg_types[arg_i];
                        argument_buffer[arg_i++] = arg;
                    }

                    auto* call = builder.create_call(reinterpret_cast<Value*>(function), { argument_buffer, arg_count });
                    return reinterpret_cast<Value*>(call);
                }
            } break;
            case NodeType::UnaryOp:
            {
                assert(node->unary_op.pos == UnaryOpType::Prefix);
                auto unary_op_type = node->unary_op.type;
                auto* unary_op_expr = node->unary_op.node;
                assert(unary_op_expr);

                switch (unary_op_type)
                {
                    case UnaryOp::AddressOf:
                    {
                        assert(unary_op_expr->type == NodeType::VarExpr);
                        auto* ref_var_decl = unary_op_expr->var_expr.mentioned;
                        assert(ref_var_decl);
                        assert(ref_var_decl->type == NodeType::VarDecl);
                        auto* ref_var_decl_type = ref_var_decl->var_decl.type;
                        assert(ref_var_decl_type);
                        auto* var_alloca = reinterpret_cast<Value*>(ref_var_decl->var_decl.backend_ref);
                        assert(var_alloca);
                        return var_alloca;
                    } break;
                    case UnaryOp::PointerDereference:
                    {
                        if (node->value_type == ValueType::LValue)
                        {
                            assert(unary_op_expr->type == NodeType::VarExpr);
                            auto* pointer_to_dereference_decl = unary_op_expr->var_expr.mentioned;
                            assert(pointer_to_dereference_decl);
                            assert(pointer_to_dereference_decl->type == NodeType::VarDecl);
                            auto* pointer_type = pointer_to_dereference_decl->var_decl.type;
                            assert(pointer_type);
                            assert(pointer_type->id == User::TypeID::PointerType);
                            auto* rns_ptr_type = get_type(allocator, builder.context, pointer_type);
                            assert(rns_ptr_type);
                            auto* pointer_alloca = reinterpret_cast<Value*>(pointer_to_dereference_decl->var_decl.backend_ref);
                            assert(pointer_alloca);
                            auto* pointer_load = builder.create_load(rns_ptr_type, pointer_alloca);
                            return reinterpret_cast<Value*>(pointer_load);
                        }
                        else
                        {
                            assert(unary_op_expr->type == NodeType::VarExpr);
                            auto* pointer_to_dereference_decl = unary_op_expr->var_expr.mentioned;
                            assert(pointer_to_dereference_decl);
                            assert(pointer_to_dereference_decl->type == NodeType::VarDecl);
                            auto* pointer_type = pointer_to_dereference_decl->var_decl.type;
                            assert(pointer_type);
                            assert(pointer_type->id == User::TypeID::PointerType);
                            auto* rns_pointer_type = get_type(allocator, builder.context, pointer_type);
                            assert(rns_pointer_type);
                            auto* pointer_alloca = reinterpret_cast<Value*>(pointer_to_dereference_decl->var_decl.backend_ref);
                            assert(pointer_alloca);
                            auto* pointer_load = builder.create_load(rns_pointer_type, pointer_alloca);
                            auto* rns_pointer_type_def = reinterpret_cast<PointerType*>(rns_pointer_type);
                            auto* deref_type = rns_pointer_type_def->type;
                            assert(deref_type);
                            auto* deref_expr = builder.create_load(deref_type, reinterpret_cast<Value*>(pointer_load));
                            return reinterpret_cast<Value*>(deref_expr);
                        }
                    } break;
                    default:
                        RNS_NOT_IMPLEMENTED;
                        break;
                }
            } break;
            case NodeType::ArrayLit:
            {
                auto count = node->array_lit.elements.len;
                assert(count > 0);
                auto* ast_type = node->array_lit.type;
                auto* array_type = get_type(allocator, builder.context, ast_type);
                assert(array_type);

                Slice<Value*> arrvalues = {
                    .ptr = new(allocator) Value* [count],
                    .len = count,
                };

                for (auto i = 0; i < count; i++)
                {
                    auto* arrnode = node->array_lit.elements[i];
                    assert(arrnode);
                    arrvalues[i] = do_node(allocator, builder, arrnode);
                    assert(arrvalues[i]);
                }

                auto* constarray = builder.context.get_constant_array(arrvalues, array_type);
                return reinterpret_cast<Value*>(constarray);
            } break;
            case NodeType::Subscript:
            {
                auto* expr = node->subscript.expr_ref;
                auto* index = node->subscript.index_ref;
                assert(expr);
                assert(index);
                auto* index_value = do_node(allocator, builder, index);
                assert(index_value);

                Value* alloca_value = nullptr;
                switch (expr->type)
                {
                    case NodeType::VarExpr:
                    {
                        auto* var_decl = expr->var_expr.mentioned;
                        assert(var_decl);
                        alloca_value = reinterpret_cast<Value*>(var_decl->var_decl.backend_ref);
                        assert(alloca_value);
                    } break;
                    default:
                        RNS_NOT_IMPLEMENTED;
                        break;
                }

                assert(alloca_value->base_id == ValueID::Instruction);
                auto* alloca_instruction = reinterpret_cast<Instruction*>(alloca_value);
                auto* alloca_type = alloca_instruction->alloca_i.allocated_type;
                assert(alloca_type);
                auto* zero_value = ConstantInt::get(allocator, builder.context.get_integer_type(32), 0, false);
                Value* indices[] = { reinterpret_cast<Value*>(zero_value), index_value };
                Slice<Value*> indices_slice = { indices, rns_array_length(indices) };
                auto* gep = builder.create_inbounds_GEP(alloca_type, alloca_value, indices_slice);
                return reinterpret_cast<Value*>(gep);
            } break;
            default:
                RNS_NOT_IMPLEMENTED;
                break;
        }

        return nullptr;
    }

    void encode(Compiler& compiler, NodeBuffer& node_buffer, FunctionTypeBuffer& function_type_declarations, FunctionDeclarationBuffer& function_declarations)
    {
        RNS_PROFILE_FUNCTION();
        compiler.subsystem = Compiler::Subsystem::IR;
        auto llvm_allocator = create_suballocator(&compiler.page_allocator, RNS_MEGABYTE(50));
        Module module = module.create(&llvm_allocator);
        BasicBlockBuffer basic_block_buffer = basic_block_buffer.create(&llvm_allocator, 1024);
        InstructionBuffer instruction_buffer = instruction_buffer.create(&llvm_allocator, 1024 * 16);
        module.functions = module.functions.create(&llvm_allocator, function_declarations.len);

        Context context = Context::create(&llvm_allocator);

        for (auto& ast_current_function : function_declarations)
        {
            auto* function_type = &ast_current_function->function.type->type_expr;
            assert(function_type->id == User::TypeID::FunctionType);
            auto* rns_function_type = get_type(&llvm_allocator, context, function_type);
            assert(rns_function_type);
            Function::create(&module, rns_function_type, ast_current_function->function.name);
        }

        for (auto i = 0; i < function_declarations.len; i++)
        {
            auto& ast_current_function = function_declarations[i];
            auto* function = &module.functions[i];
            Builder builder = { .context = context, };
            builder.basic_block_buffer = &basic_block_buffer;
            builder.instruction_buffer = &instruction_buffer;
            builder.function = function;
            builder.module = &module;

            auto* ast_main_scope = ast_current_function->function.scope_blocks[0];
            auto& ast_main_scope_statements = ast_main_scope->block.statements;
            builder.function->basic_blocks = builder.function->basic_blocks.create(&llvm_allocator, 128);

            BasicBlock* entry_block = builder.create_block(&llvm_allocator);
            builder.append_to_function(entry_block);
            builder.set_block(entry_block);

            function->arguments.len = ast_current_function->function.arguments.len;
            auto* function_base_type = builder.function->type;
            auto* function_type = reinterpret_cast<FunctionType*>(function_base_type);
            auto ret_type = function_type->ret_type;
            assert(ret_type);

            bool ret_type_void = ret_type->id == TypeID::Void;
            builder.explicit_return = false;

            for (auto* st_node : ast_main_scope_statements)
            {
                if (st_node->type == NodeType::Conditional)
                {
                    if (introspect_for_conditional_allocas(st_node->conditional.if_block))
                    {
                        builder.explicit_return = true;
                        break;
                    }
                    if (introspect_for_conditional_allocas(st_node->conditional.else_block))
                    {
                        builder.explicit_return = true;
                        break;
                    }
                }
                else if (st_node->type == NodeType::Loop)
                {
                    if (introspect_for_conditional_allocas(st_node->loop.body))
                    {
                        builder.explicit_return = true;
                        break;
                    }
                }
                // @Warning: here we need to comtemplate other cases which imply new blocks
            }
            builder.conditional_alloca = !ret_type_void && builder.explicit_return;

            //auto alloca_count = arg_count + ast_current_function->function.variables.len + conditional_alloca;

            // @TODO: reserve as many position as 'alloca_count' in the main basic block, displace the len by that offset and write the rest of instructions there.
            // Alloca can have their special insertion entry point
            // @WARNING: this would imply we could fail with non-optimized build dead code elimination

            if (builder.explicit_return)
            {
                builder.exit_block = builder.create_block(&llvm_allocator);
            }
            if (builder.conditional_alloca)
            {
                builder.return_alloca = builder.create_alloca(ret_type);
            }

            // Arguments
            if (function->arguments.len)
            {
                function->arguments.ptr = new (&llvm_allocator) Argument[function->arguments.len];
                assert(function->arguments.ptr);
                auto arg_index = 0;
                for (auto* arg_node : ast_current_function->function.arguments)
                {
                    assert(arg_node->type == NodeType::VarDecl);
                    auto* arg_type = arg_node->var_decl.type;
                    assert(arg_type);
                    auto* rns_arg_type = get_type(&llvm_allocator, context, arg_type);
                    assert(rns_arg_type);
                    assert(arg_node->var_decl.is_fn_arg);
                    auto arg_name = arg_node->var_decl.name;

                    auto* arg = &function->arguments[arg_index++];
                    *arg = {
                        .value = {
                            .type = rns_arg_type,
                            .base_id = ValueID::Argument,
                        },
                        .arg_index = arg_index,
                    };

                    auto* arg_alloca = builder.create_alloca(rns_arg_type);
                    arg_node->var_decl.backend_ref = arg_alloca;

                    builder.create_store(reinterpret_cast<Value*>(arg), reinterpret_cast<Value*>(arg_alloca));
                }
            }

            do_node(&llvm_allocator, builder, ast_main_scope);

            if (builder.conditional_alloca)
            {
                assert(builder.current->instructions.len != 0);

                builder.append_to_function(builder.exit_block);
                builder.set_block(builder.exit_block);

                auto* loaded_return = builder.create_load(builder.return_alloca->alloca_i.allocated_type, reinterpret_cast<Value*>(builder.return_alloca));
                assert(loaded_return);
                builder.create_ret(reinterpret_cast<Value*>(loaded_return));
            }
            else if (ret_type_void)
            {
                if (builder.explicit_return)
                {
                    if (builder.current->instructions.len == 0)
                    {
                        auto* saved_current = builder.set_block(builder.exit_block);
                        assert(saved_current);
                        auto index = builder.function->basic_blocks.get_id_if_ref_buffer(saved_current);
                        // @Info: this is a no-statements function.
                        // @TODO: not create a basic block if the function has no statements
                        assert(index == 0);
                        builder.function->basic_blocks[index] = builder.current;
                        builder.current->parent = builder.function;
                    }
                    else
                    {
                        builder.append_to_function(builder.exit_block);
                        builder.set_block(builder.exit_block);
                    }
                }

                builder.create_ret_void();
            }

            function->print(&llvm_allocator);
        }
    }
}
