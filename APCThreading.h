#pragma once

#include <Windows.h>
#include <cstdint>

typedef struct _APC_TASK_CALLBACKS
{
    void* ExecuteCallback;
    void* Unposted;
} APC_TASK_CALLBACKS, * PAPC_TASK_CALLBACKS;

typedef struct _APC_TASK
{
    struct _APC_TASK_CALLBACKS* Callbacks;
    UINT32 NumaNode;
    UINT8 IdealProcessor;
    char Padding_242[3];
    struct _LIST_ENTRY ListEntry;
} APC_TASK, * PAPC_TASK;

typedef struct _APCP_REFCOUNT
{
    volatile INT32 Refcount;
} APCP_REFCOUNT, * PAPCP_REFCOUNT;

typedef struct _APCP_CALLER
{
    void* ReturnAddress;
} APCP_CALLER, * PAPCP_CALLER;

typedef struct _APCP_PH
{
    struct _APCP_PH_LINKS* Root;
} APCP_PH, * PAPCP_PH;

typedef struct _APC_DIRECT
{
    struct _APC_TASK Task;
    UINT64 Lock;
    struct _LIST_ENTRY IoCompletionInformationList;
    void* Callback;
    UINT32 NumaNode;
    UINT8 IdealProcessor;
    char __PADDING__[3];
} APC_DIRECT, * PAPC_DIRECT;

typedef struct _APCP_TIMER_SUBQUEUE
{
    INT64 Expiration;
    struct _APCP_PH WindowStart;
    struct _APCP_PH WindowEnd;
    void* Timer;
    void* TimerPkt;
    struct _APC_DIRECT Direct;
    UINT32 ExpirationWindow;
    INT32 __PADDING__[1];
} APCP_TIMER_SUBQUEUE, * PAPCP_TIMER_SUBQUEUE;

typedef struct _APCP_TIMER_QUEUE
{
    struct _RTL_SRWLOCK Lock;
    struct _APCP_TIMER_SUBQUEUE AbsoluteQueue;
    struct _APCP_TIMER_SUBQUEUE RelativeQueue;
    INT32 AllocatedTimerCount;
    INT32 __PADDING__[1];
} APCP_TIMER_QUEUE, * PAPCP_TIMER_QUEUE;

typedef struct _APCP_NUMA_NODE
{
    INT32 WorkerCount;
} APCP_NUMA_NODE, * PAPCP_NUMA_NODE;

typedef union _APCP_POOL_QUEUE_STATE
{
    union
    {
        INT64 Exchange;
        struct
        {
            INT32 RunningThreadGoal : 16;
            UINT32 PendingReleaseCount : 16;
            UINT32 QueueLength;
        };
    };
} APCP_POOL_QUEUE_STATE, * PAPCP_POOL_QUEUE_STATE;

typedef struct _APCP_QUEUE
{
    struct _LIST_ENTRY Queue;
    struct _RTL_SRWLOCK Lock;
} APCP_QUEUE, * PAPCP_QUEUE;

typedef struct _FULL_APC_POOL
{
    struct _APCP_REFCOUNT Refcount;
    long Padding_239;
    union _APCP_POOL_QUEUE_STATE QueueState;
    struct _APCP_QUEUE* TaskQueue[3];
    struct _APCP_NUMA_NODE* NumaNode;
    struct _GROUP_AFFINITY* ProximityInfo;
    void* WorkerFactory;
    void* CompletionPort;
    struct _RTL_SRWLOCK Lock;
    struct _LIST_ENTRY PoolObjectList;
    struct _LIST_ENTRY WorkerList;
    struct _APCP_TIMER_QUEUE TimerQueue;
    struct _RTL_SRWLOCK ShutdownLock;
    UINT8 ShutdownInitiated;
    UINT8 Released;
    UINT16 PoolFlags;
    long Padding_240;
    struct _LIST_ENTRY PoolLinks;
    struct _APCP_CALLER AllocCaller;
    struct _APCP_CALLER ReleaseCaller;
    volatile INT32 AvailableWorkerCount;
    volatile INT32 LongRunningWorkerCount;
    UINT32 LasAPCrocCount;
    volatile INT32 NodeStatus;
    volatile INT32 BindingCount;
    UINT32 CallbackChecksDisabled : 1;
    UINT32 TrimTarget : 11;
    UINT32 TrimmedThrdCount : 11;
    UINT32 SelectedCpuSetCount;
    long Padding_241;
    struct _RTL_CONDITION_VARIABLE TrimComplete;
    struct _LIST_ENTRY TrimmedWorkerList;
} FULL_APC_POOL, * PFULL_APC_POOL;

typedef struct _ALPC_WORK_ON_BEHALF_TICKET
{
    UINT32 ThreadId;
    UINT32 ThreadCreationTimeLow;
} ALPC_WORK_ON_BEHALF_TICKET, * PALPC_WORK_ON_BEHALF_TICKET;

typedef union _APCP_WORK_STATE
{
    union
    {
        INT32 Exchange;
        UINT32 Insertable : 1;
        UINT32 PendingCallbackCount : 31;
    };
} APCP_WORK_STATE, * PAPCP_WORK_STATE;

typedef struct _APCP_ITE_WAITER
{
    struct _APCP_ITE_WAITER* Next;
    void* ThreadId;
} APCP_ITE_WAITER, * PAPCP_ITE_WAITER;

typedef struct _APCP_PH_LINKS
{
    struct _LIST_ENTRY Siblings;
    struct _LIST_ENTRY Children;
    INT64 Key;
} APCP_PH_LINKS, * PAPCP_PH_LINKS;

typedef struct _APCP_ITE
{
    struct _APCP_ITE_WAITER* First;
} APCP_ITE, * PAPCP_ITE;

typedef union _APCP_FLAGS_COUNT
{
    union
    {
        UINT64 Count : 60;
        UINT64 Flags : 4;
        INT64 Data;
    };
} APCP_FLAGS_COUNT, * PAPCP_FLAGS_COUNT;

typedef struct _APCP_BARRIER
{
    volatile union _APCP_FLAGS_COUNT Ptr;
    struct _RTL_SRWLOCK WaitLock;
    struct _APCP_ITE WaitList;
} APCP_BARRIER, * PAPCP_BARRIER;

typedef struct _APC_CLEANUP_GROUP
{
    struct _APCP_REFCOUNT Refcount;
    INT32 Released;
    struct _RTL_SRWLOCK MemberLock;
    struct _LIST_ENTRY MemberList;
    struct _APCP_BARRIER Barrier;
    struct _RTL_SRWLOCK CleanupLock;
    struct _LIST_ENTRY CleanupList;
} APC_CLEANUP_GROUP, * PAPC_CLEANUP_GROUP;


typedef struct _APCP_CLEANUP_GROUP_MEMBER
{
    struct _APCP_REFCOUNT Refcount;
    long Padding_233;
    const struct _APCP_CLEANUP_GROUP_MEMBER_VFUNCS* VFuncs;
    struct _APC_CLEANUP_GROUP* CleanupGroup;
    void* CleanupGroupCancelCallback;
    void* FinalizationCallback;
    struct _LIST_ENTRY CleanupGroupMemberLinks;
    struct _APCP_BARRIER CallbackBarrier;
    union
    {
        void* Callback;
        void* WorkCallback;
        void* SimpleCallback;
        void* TimerCallback;
        void* WaitCallback;
        void* IoCallback;
        void* AlpcCallback;
        void* AlpcCallbackEx;
        void* JobCallback;
    };
    void* Context;
    struct _ACTIVATION_CONTEXT* ActivationContext;
    void* SubProcessTag;
    struct _GUID ActivityId;
    struct _ALPC_WORK_ON_BEHALF_TICKET WorkOnBehalfTicket;
    void* RaceDll;
    FULL_APC_POOL* Pool;
    struct _LIST_ENTRY PoolObjectLinks;
    union
    {
        volatile INT32 Flags;
        UINT32 LongFunction : 1;
        UINT32 Persistent : 1;
        UINT32 UnusedPublic : 14;
        UINT32 Released : 1;
        UINT32 CleanupGroupReleased : 1;
        UINT32 InCleanupGroupCleanupList : 1;
        UINT32 UnusedPrivate : 13;
    };
    long Padding_234;
    struct _APCP_CALLER AllocCaller;
    struct _APCP_CALLER ReleaseCaller;
    enum _APC_CALLBACK_PRIORITY CallbackPriority;
    INT32 __PADDING__[1];
} APCP_CLEANUP_GROUP_MEMBER, * PAPCP_CLEANUP_GROUP_MEMBER;

typedef struct _FULL_APC_WORK
{
    struct _APCP_CLEANUP_GROUP_MEMBER CleanupGroupMember;
    struct _APC_TASK Task;
    volatile union _APCP_WORK_STATE WorkState;
    INT32 __PADDING__[1];
} FULL_APC_WORK, * PFULL_APC_WORK;

typedef struct _FULL_APC_TIMER
{
    struct _FULL_APC_WORK Work;
    struct _RTL_SRWLOCK Lock;
    union
    {
        struct _APCP_PH_LINKS WindowEndLinks;
        struct _LIST_ENTRY ExpirationLinks;
    };
    struct _APCP_PH_LINKS WindowStartLinks;
    INT64 DueTime;
    struct _APCP_ITE Ite;
    UINT32 Window;
    UINT32 Period;
    UINT8 Inserted;
    UINT8 WaitTimer;
    union
    {
        UINT8 TimerStatus;
        UINT8 InQueue : 1;
        UINT8 Absolute : 1;
        UINT8 Cancelled : 1;
    };
    UINT8 BlockInsert;
    INT32 __PADDING__[1];
} FULL_APC_TIMER, * PFULL_APC_TIMER;

typedef struct _FULL_APC_WAIT
{
    struct _FULL_APC_TIMER Timer;
    void* Handle;
    void* WaiAPCkt;
    void* NextWaitHandle;
    union _LARGE_INTEGER NextWaitTimeout;
    struct _APC_DIRECT Direct;
    union
    {
        union
        {
            UINT8 AllFlags;
            UINT8 NextWaitActive : 1;
            UINT8 NextTimeoutActive : 1;
            UINT8 CallbackCounted : 1;
            UINT8 Spare : 5;
        };
    } WaitFlags;
    char __PADDING__[7];
} FULL_APC_WAIT, * PFULL_APC_WAIT;

typedef struct _FULL_APC_IO
{
    struct _APCP_CLEANUP_GROUP_MEMBER CleanupGroupMember;
    struct _APC_DIRECT Direct;
    void* File;
    volatile INT32 PendingIrpCount;
    INT32 __PADDING__[1];
} FULL_APC_IO, * PFULL_APC_IO;

typedef struct _FULL_APC_ALPC
{
    struct _APC_DIRECT Direct;
    struct _APCP_CLEANUP_GROUP_MEMBER CleanupGroupMember;
    void* AlpcPort;
    INT32 DeferredSendCount;
    INT32 LastConcurrencyCount;
    union
    {
        UINT32 Flags;
        UINT32 ExTypeCallback : 1;
        UINT32 CompletionListRegistered : 1;
        UINT32 Reserved : 30;
    };
    INT32 __PADDING__[1];
} FULL_APC_ALPC, * PFULL_APC_ALPC;

typedef struct _FULL_APC_JOB
{
    struct _APC_DIRECT Direct;
    struct _APCP_CLEANUP_GROUP_MEMBER CleanupGroupMember;
    void* JobHandle;
    union
    {
        volatile int64_t CompletionState;
        int64_t Rundown : 1;
        int64_t CompletionCount : 63;
    };
    struct _RTL_SRWLOCK RundownLock;
} FULL_APC_JOB, * PFULL_APC_JOB;

typedef VOID(NTAPI* PAPC_ALPC_CALLBACK)(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context,
    _In_ PFULL_APC_ALPC Alpc
    );
