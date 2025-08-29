#include <stdio.h>
#include <papi.h>

#define NUM_EVENTS 1
#define ITERATIONS 1000000

// 简单的函数，包含一个循环
void simple_function() {
    int i;
    for (i = 0; i < ITERATIONS; i++) {
        // 这里可以是任何需要测试性能的代码
        // 为了简单起见，这里只是一个空循环
    }
}

int main() {
    int EventSet = PAPI_NULL;
    long long values[NUM_EVENTS];
    int ret;

    // 初始化 PAPI 库
    ret = PAPI_library_init(PAPI_VER_CURRENT);
    if (ret != PAPI_VER_CURRENT) {
        fprintf(stderr, "PAPI 库初始化失败: %d\n", ret);
        return 1;
    }

    // 创建一个事件集
    ret = PAPI_create_eventset(&EventSet);
    if (ret != PAPI_OK) {
        fprintf(stderr, "创建事件集失败: %d\n", ret);
        return 1;
    }

    // 将 PAPI_TOT_CYC 事件添加到事件集中
    ret = PAPI_add_event(EventSet, PAPI_TOT_CYC);
    if (ret != PAPI_OK) {
        fprintf(stderr, "添加事件失败: %d\n", ret);
        return 1;
    }

    // 开始记录性能数据
    ret = PAPI_start(EventSet);
    if (ret != PAPI_OK) {
        fprintf(stderr, "开始记录失败: %d\n", ret);
        return 1;
    }

    // 调用需要测试性能的函数
    simple_function();

    // 停止记录性能数据
    ret = PAPI_stop(EventSet, values);
    if (ret != PAPI_OK) {
        fprintf(stderr, "停止记录失败: %d\n", ret);
        return 1;
    }

    // 输出记录的总 CPU 周期数
    printf("执行 %d 次迭代消耗的总 CPU 周期数: %lld\n", ITERATIONS, values[0]);

    // 清理事件集
    ret = PAPI_cleanup_eventset(EventSet);
    if (ret != PAPI_OK) {
        fprintf(stderr, "清理事件集失败: %d\n", ret);
        return 1;
    }

    // 销毁事件集
    ret = PAPI_destroy_eventset(&EventSet);
    if (ret != PAPI_OK) {
        fprintf(stderr, "销毁事件集失败: %d\n", ret);
        return 1;
    }

    return 0;
}
