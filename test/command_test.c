#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../commands/command_.h"

// 测试命令解析函数
void test_tokenize_command() {
    printf("测试命令解析函数...\n");

    // 测试空命令
    {
        char line1[] = "";
        char *argv[10] = {NULL};
        unsigned argc = tokenize_command(line1, argv, 10);
        assert(argc == 0);
        printf("空命令测试通过\n");
        for (int i = 0; i < argc; i++) {
            free(argv[i]);
        }
    }

    // 测试单个命令
    {
        char line2[] = "GET";
        char *argv[10] = {NULL};
        unsigned argc = tokenize_command(line2, argv, 10);
        assert(argc == 1);
        assert(strcmp(argv[0], "GET") == 0);
        printf("单个命令测试通过\n");
        for (int i = 0; i < argc; i++) {
            free(argv[i]);
        }
    }

    // 测试带一个参数的命令
    {
        char line3[] = "GET key1";
        char *argv[10] = {NULL};
        unsigned argc = tokenize_command(line3, argv, 10);
        assert(argc == 2);
        assert(strcmp(argv[0], "GET") == 0);
        assert(strcmp(argv[1], "key1") == 0);
        for (unsigned i = 0; i < argc; i++) {
            free(argv[i]);
        }
        printf("带一个参数的命令测试通过\n");
    }

    // 测试带多个参数的命令
    {
        char line4[] = "SET key1 value1 3600";
        char *argv[10] = {NULL};
        unsigned argc = tokenize_command(line4, argv, 10);
        assert(argc == 4);
        assert(strcmp(argv[0], "SET") == 0);
        assert(strcmp(argv[1], "key1") == 0);
        assert(strcmp(argv[2], "value1") == 0);
        assert(strcmp(argv[3], "3600") == 0);
        for (unsigned i = 0; i < argc; i++) {
            free(argv[i]);
        }
        printf("带多个参数的命令测试通过\n");
    }

    // 测试前导空格
    {
        char line5[] = "  SET key1 value1";
        char *argv[10] = {NULL};
        unsigned argc = tokenize_command(line5, argv, 10);
        assert(argc == 3);
        assert(strcmp(argv[0], "SET") == 0);
        assert(strcmp(argv[1], "key1") == 0);
        assert(strcmp(argv[2], "value1") == 0);
        for (unsigned i = 0; i < argc; i++) {
            free(argv[i]);
        }
        printf("前导空格测试通过\n");
    }

    // 测试多余空格
    {
        char line6[] = "SET   key1    value1  ";
        char *argv[10] = {NULL};
        unsigned argc = tokenize_command(line6, argv, 10);
        assert(argc == 3);
        assert(strcmp(argv[0], "SET") == 0);
        assert(strcmp(argv[1], "key1") == 0);
        assert(strcmp(argv[2], "value1") == 0);
        for (unsigned i = 0; i < argc; i++) {
            free(argv[i]);
        }
        printf("多余空格测试通过\n");
    }

    // 测试参数超出限制
    {
        char line7[] = "CMD arg1 arg2 arg3 arg4 arg5 arg6";
        char *argv[3] = {NULL};
        unsigned argc = tokenize_command(line7, argv, 3);
        assert(argc == 3);  // 只应解析前3个参数
        assert(strcmp(argv[0], "CMD") == 0);
        assert(strcmp(argv[1], "arg1") == 0);
        assert(strcmp(argv[2], "arg2") == 0);
        for (unsigned i = 0; i < argc; i++) {
            free(argv[i]);
        }
        printf("参数超出限制测试通过\n");
    }

    printf("tokenize_command() 所有测试通过!\n\n");
}

// 测试命令匹配函数
void test_match() {
    printf("测试命令匹配函数...\n");

    // 测试匹配现有命令
    {
        Command *cmd = match("SET");
        assert(cmd != NULL);
        assert(strcmp(cmd->name, "SET") == 0);
        assert(cmd->min_args == 2);
        assert(cmd->max_args == 3);
        printf("匹配SET命令测试通过\n");
    }

    {
        Command *cmd = match("GET");
        assert(cmd != NULL);
        assert(strcmp(cmd->name, "GET") == 0);
        assert(cmd->min_args == 1);
        assert(cmd->max_args == 1);
        printf("匹配GET命令测试通过\n");
    }

    // 测试不区分大小写
    {
        Command *cmd = match("set");
        assert(cmd != NULL);
        assert(strcmp(cmd->name, "SET") == 0);
        printf("不区分大小写测试通过\n");
    }

    // 测试匹配不存在的命令
    {
        Command *cmd = match("NONEXIST");
        assert(cmd == NULL);
        printf("匹配不存在命令测试通过\n");
    }

    printf("match() 所有测试通过!\n\n");
}

// 测试命令整体流程
void test_command_workflow() {
    printf("测试命令整体流程...\n");

    // 初始化哈希表
    HashTable table;
    init(&table, 10, free);

    // 处理SET命令
    {
        char line[] = "SET testkey testvalue";
        char *argv[10] = {NULL};
        unsigned argc = tokenize_command(line, argv, 10);

        Command *cmd = match(argv[0]);
        assert(cmd != NULL);

        // 验证参数数量
        assert(argc - 1 >= cmd->min_args && (cmd->max_args == -1 || argc - 1 <= cmd->max_args));

        // 执行命令
        CommandResponse *resp = cmd->handler(argc - 1, (const char **)&argv[1], &table);
        assert(resp->type == RESP_OK);

        // 清理资源
        for (unsigned i = 0; i < argc; i++) {
            free(argv[i]);
        }
        free_response(resp);
        printf("SET命令流程测试通过\n");
    }

    // 处理GET命令
    {
        char line[] = "GET testkey";
        char *argv[10] = {NULL};
        unsigned argc = tokenize_command(line, argv, 10);

        Command *cmd = match(argv[0]);
        assert(cmd != NULL);

        // 验证参数数量
        assert(argc - 1 >= cmd->min_args && (cmd->max_args == -1 || argc - 1 <= cmd->max_args));

        // 执行命令
        CommandResponse *resp = cmd->handler(argc - 1, (const char **)&argv[1], &table);
        assert(resp->type == RESP_OK);
        assert(resp->data != NULL);
        assert(strcmp((char*)resp->data, "testvalue") == 0);

        // 清理资源
        for (unsigned i = 0; i < argc; i++) {
            free(argv[i]);
        }
        free_response(resp);
        printf("GET命令流程测试通过\n");
    }

    // 处理无效参数数量的命令
    {
        char line[] = "GET";  // 缺少key参数
        char *argv[10] = {NULL};
        unsigned argc = tokenize_command(line, argv, 10);

        Command *cmd = match(argv[0]);
        assert(cmd != NULL);

        // 验证参数数量
        assert(argc - 1 < cmd->min_args);

        // 清理资源
        for (unsigned i = 0; i < argc; i++) {
            free(argv[i]);
        }
        printf("无效参数数量测试通过\n");
    }

    // 清理哈希表
    clear(&table);

    printf("命令整体流程测试通过!\n\n");
}
//
// int main() {
//     printf("开始测试命令解析和匹配功能...\n\n");
//
//     test_tokenize_command();
//     test_match();
//     test_command_workflow();
//
//     printf("所有测试通过!\n");
//     return 0;
// }