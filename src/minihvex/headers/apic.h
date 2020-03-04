#ifndef _APIC_H_
#define _APIC_H_

#include "minihv.h"
#include "acpi_interface.h"
#include "event.h"

#define PREDEFINED_ICR_SIZE                     4
#define PREDEFINED_LAPIC_SIZE                   0x400

#define APIC_SPURIOUS_SUPPRESS_EOI              (1<<12)

#define APIC_PROCESSOR_ACTIVE                   0x1

#define IA32_APIC_BASE_MASK(reg)                (((QWORD)(reg))&0xFFFFFF000)
#define IA32_APIC_BASE_ENABLE_FLAG              (1<<11)     // if set => APIC enabled
#define IA32_APIC_EXT_ENABLE_FLAG               (1<<10)        // x2 APIC enable
#define IA32_APIC_BSP_FLAG                      (1<<8)

// APIC registers
#define APIC_LAPIC_ID_REG_OFFSET                0x020
#define APIC_TPR_REG_OFFSET                     0x080
#define APIC_EOI_REG_OFFSET                     0x0B0
#define APIC_LOGIC_DEST_REG_OFFSET              0x0D0
#define APIC_DEST_FORM_REG_OFFSET               0x0E0
#define APIC_SPURIOUS_VECTOR_REG_OFFSET         0x0F0
#define APIC_ICR_LOW_REG_OFFSET                 0x300
#define APIC_ICR_HIGH_REG_OFFSET                0x310
#define APIC_LVT_TIMER_CONTROL_REG_OFFSET       0x320
#define APIC_LVT_PERF_REG_OFFSET                0x340
#define APIC_LVT_LINT0_REG_OFFSET               0x350
#define APIC_LVT_LINT1_REG_OFFSET               0x360
#define APIC_INITIAL_COUNT_REG                  0x380
#define APIC_CURRENT_COUNT_REG                  0x390
#define APIC_DIVIDE_CONTROL_REG_OFFSET          0x3E0

// APIC_SPURIOUS_VECTOR_REG_OFFSET
#define APIC_SOFTWARE_ENABLE                    (1<<8)

// APIC_LVT_TIMER_CONTROL_REG_OFFSET related flags
#define APIC_TIMER_ONE_SHOT                     ((0x00)<<17)
#define APIC_TIMER_PERIODIC                     ((0x01)<<17)
#define APIC_TIMER_TSC_DEADLINE                 ((0x10)<<17)

#define APIC_INTERRUPT_MASK                     (0x1<<16)

#define APIC_FREQUENCY_DIV_ONE                  0xB         // 0b1011

#define INIT_SLEEP                              (10*MS_IN_US)   // 10 ms = 10000us sleep
#define SIPI_SLEEP                              (200)           // 200 us sleep

typedef enum _APIC_DELIVERY_MODE
{
    ApicDeliveryModeFixed,
    ApicDeliveryModeLowest,
    ApicDeliveryModeSMI = 2,
    ApicDeliveryModeNMI = 4,
    ApicDeliveryModeINIT,
    ApicDeliveryModeSIPI
} APIC_DELIVERY_MODE;

typedef enum _APIC_DESTINATION_SHORTHAND
{
    ApicDestinationShorthandNone,
    ApicDestinationShorthandSelf,
    ApicDestinationShorthandAll,
    ApicDestinationShorthandAllExcludingSelf
} APIC_DESTINATION_SHORTHAND;

#pragma pack(push,1)
typedef struct _ICR_HIGH_REGISTER
{
    DWORD           Reserved                :   24;
    DWORD           Destination             :   8;
} ICR_HIGH_REGISTER, *PICR_HIGH_REGISTER;
STATIC_ASSERT(sizeof(ICR_HIGH_REGISTER) == PREDEFINED_ICR_SIZE);

typedef struct _ICR_LOW_REGISTER
{
    DWORD           Vector                  :   8;
    DWORD           DeliveryMode            :   3;
    DWORD           DestinationMode         :   1;
    DWORD           DeliveryStatus          :   1;
    DWORD           Reserved0               :   1;
    DWORD           Level                   :   1;
    DWORD           TriggerMode             :   1;
    DWORD           Reserved1               :   2;
    DWORD           DestinationShorthand    :   2;
    DWORD           Reserved2               :  12;
} ICR_LOW_REGISTER, *PICR_LOW_REGISTER;
STATIC_ASSERT(sizeof(ICR_LOW_REGISTER) == PREDEFINED_ICR_SIZE);
#pragma pack(pop)


typedef struct _APIC_DATA
{
    // The base address of the APIC
    // This is used so we don't have to read a MSR each time and mask the address.
    PVOID            ApicBaseAddress;

    // Number of currently active CPUs
    volatile DWORD   ActiveCpus;

    // Lock which is used to 'signal' when an AP has finished initialization
    EVENT            WakeupLock;

    // The list of physical CPUs
    LIST_ENTRY       PhysCpuListHead;
} APIC_DATA, *PAPIC_DATA;

//******************************************************************************
// Function:    ApicMapRegister
// Description: This function MUST be called before any other APIC function.
// Returns:       STATUS
// Parameter:     void
//******************************************************************************
STATUS
ApicMapRegister(
    void
    );

//******************************************************************************
// Function:      ApicWakeUpCpus
// Description: This method receives the list of Physical CPUs and wakes them
//              one by one.
// Returns:       void
// Parameter:     IN PLIST_ENTRY PhysicalCpuList
//******************************************************************************
void
ApicWakeUpCpus(
    IN     PLIST_ENTRY     PhysicalCpuList       
);

//******************************************************************************
// Function:      ApicInitAPCpu
// Description: This function is called from YASM and is the bridge to C.
//              Each AP will call this function exactly once.
// Returns:       void
// Parameter:     void
//******************************************************************************
void
ApicInitAPCpu(
    void        
    );


//******************************************************************************
// Function:    ApicSendIpi
// Description: Sends an Interprocessor-Interrupt (IPI) to another processor.
// Returns:       void
// Parameter:     IN BYTE ApicId
// Parameter:     IN APIC_DELIVERY_MODE DeliveryMode
// Parameter:     IN APIC_DESTINATION_SHORTHAND DestinationShorthand
// Parameter:     IN_OPT BYTE * Vector
//******************************************************************************
void
ApicSendIpi(
    IN      BYTE                            ApicId,
    IN      APIC_DELIVERY_MODE              DeliveryMode,
    IN      APIC_DESTINATION_SHORTHAND      DestinationShorthand,
    IN_OPT  BYTE*                           Vector
);


//******************************************************************************
// Function:      ChangeStack
// Description: This function is exported from YASM. Switches the stack pointer
//              to the new stack preserving all the information.
// Returns:       void
// Parameter:     IN PVOID InitialStackBase - The base address of the current stack
// Parameter:   IN PVOID NewStackBase - The base address of the new stack
//******************************************************************************
typedef
void
(__cdecl *CHANGE_STACK)(
    IN  PVOID       InitialStackBase,
    IN  PVOID       NewStackBase
    );

// Assembly exported function to change the old stack to a new one.
extern CHANGE_STACK     __changeStack;

#endif // _APIC_H_