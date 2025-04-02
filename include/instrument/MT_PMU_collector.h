

#ifndef MATRIX_PMU_COLLECTOR_C_MT_PMU_COLLECTOR_H
#define MATRIX_PMU_COLLECTOR_C_MT_PMU_COLLECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PMU_NUM 26
#define MAX_EVENTSET_NUM 24

#define PMU_NULL (-1)

typedef enum {
    PMU_OK      = 0,
    PMU_EINVAL  = -1,
    PMU_ENOMEM  = -2,
    PMU_ESYS    = -3,
    PMU_EBIGSET = -4,
    PMU_ENOEVNT = -7,
    PMU_EISRUN  = -10
}pmu_api_status_t;

typedef struct {
    int eventNum;
    int events[MAX_PMU_NUM];
    int running;
}ESInfo;

void MT_PMU_collector_init();
void MT_PMU_collector_fin();
void MT_PMU_collector_get_all(unsigned long*);
int MT_PMU_collector_get_num();

pmu_api_status_t PMU_library_init        ();
pmu_api_status_t PMU_add_event           (int index, int Event);
pmu_api_status_t PMU_add_events          (int EventSet, int *Events, int number);
pmu_api_status_t PMU_add_named_event     (int EventSet, const char *EventName);
pmu_api_status_t PMU_create_eventset     (int *EventSet);
pmu_api_status_t PMU_destroy_eventset    (int *EventSet);
pmu_api_status_t PMU_cleanup_eventset    (int EventSet);

pmu_api_status_t PMU_start               (int EventSet);
pmu_api_status_t PMU_stop                (int EventSet, unsigned long *values);
pmu_api_status_t PMU_read                (int EventSet, unsigned long *values);
pmu_api_status_t PMU_reset               (int EventSet);
void PMU_shutdown();

#include <string.h>

static const char *code2name[26] =
        {"CYCLE", "BRTK", "IACK", "EXEP", "L1DRWM", "L1DRWH", "L1PRM", "L1PRH", "DPStall",
         "VMStall", "SMCSStall", "STALL", "NOP", "NonNOP", "EACK", "SIEU", "SMAC1",
         "SMAC2", "SBR", "SLD", "VIEU", "VMAC1", "VMAC2", "VMAC3", "VLS0", "VLS1"};

static pmu_api_status_t PMU_event_name_to_code(const char *EventName, int *EventCode) {
    if (strcmp(EventName, "CYCLE") == 0) *EventCode = 0;
    else if (strcmp(EventName, "BRTK") == 0) *EventCode = 1;
    else if (strcmp(EventName, "IACK") == 0) *EventCode = 2;
    else if (strcmp(EventName, "EXEP") == 0) *EventCode = 3;
    else if (strcmp(EventName, "L1DRWM") == 0) *EventCode = 4;
    else if (strcmp(EventName, "L1DRWH") == 0) *EventCode = 5;
    else if (strcmp(EventName, "L1PRM") == 0) *EventCode = 6;
    else if (strcmp(EventName, "L1PRH") == 0) *EventCode = 7;
    else if (strcmp(EventName, "DPStall") == 0) *EventCode = 8;
    else if (strcmp(EventName, "VMStall") == 0) *EventCode = 9;
    else if (strcmp(EventName, "SMCSStall") == 0) *EventCode = 10;
    else if (strcmp(EventName, "STALL") == 0) *EventCode = 11;
    else if (strcmp(EventName, "NOP") == 0) *EventCode = 12;
    else if (strcmp(EventName, "NonNOP") == 0) *EventCode = 13;
    else if (strcmp(EventName, "EACK") == 0) *EventCode = 14;
    else if (strcmp(EventName, "SIEU") == 0) *EventCode = 15;
    else if (strcmp(EventName, "SMAC1") == 0) *EventCode = 16;
    else if (strcmp(EventName, "SMAC2") == 0) *EventCode = 17;
    else if (strcmp(EventName, "SBR") == 0) *EventCode = 18;
    else if (strcmp(EventName, "SLD") == 0) *EventCode = 19;
    else if (strcmp(EventName, "VIEU") == 0) *EventCode = 20;
    else if (strcmp(EventName, "VMAC1") == 0) *EventCode = 21;
    else if (strcmp(EventName, "VMAC2") == 0) *EventCode = 22;
    else if (strcmp(EventName, "VMAC3") == 0) *EventCode = 23;
    else if (strcmp(EventName, "VLS0") == 0) *EventCode = 24;
    else if (strcmp(EventName, "VLS1") == 0) *EventCode = 25;
    else {
        *EventCode = -1;
        return PMU_ENOEVNT;
    }
    return PMU_OK;
}

static pmu_api_status_t PMU_event_code_to_name(int EventCode, char *EventName) {
    if (EventCode < 0 || EventCode >= 26) {
        strcpy(EventName, "Invalid Event Code");
        return PMU_ENOEVNT;
    }
    strcpy(EventName, code2name[EventCode]);
    return PMU_OK;
}

#ifdef __cplusplus
}
#endif

#endif //MATRIX_PMU_COLLECTOR_C_MT_PMU_COLLECTOR_H
