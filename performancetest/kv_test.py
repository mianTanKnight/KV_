#!/usr/bin/env python3
import socket
import time
import threading
import argparse
from queue import Queue
import struct

class KVBenchmark:
    def __init__(self, host='localhost', port=7000, threads=4, total_ops=100000,
                 batch_size=1000, value_size=100, test_type='set', use_binary_prefix=False):
        self.host = host
        self.port = port
        self.threads = threads
        self.total_ops = total_ops
        self.batch_size = batch_size
        self.value_size = value_size
        self.test_type = test_type
        self.ops_per_thread = total_ops // threads
        self.results_queue = Queue()
        self.use_binary_prefix = use_binary_prefix

    def worker(self, thread_id):
        """工作线程执行测试操作"""
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            sock.connect((self.host, self.port))

            start_time = time.time()
            success_count = 0
            batch_start_time = start_time

            # 预生成固定大小的值
            base_value = "v" * self.value_size

            # 使用一个连接执行多个操作，减少连接开销
            for i in range(self.ops_per_thread):
                key = f"bench_key_{thread_id}_{i}"
                value = base_value  # 使用预生成的固定大小值

                if self.test_type == 'set':
                    cmd = f"SET {key} {value}"
                elif self.test_type == 'get':
                    cmd = f"GET {key}"
                elif self.test_type == 'del':
                    cmd = f"DEL {key}"
                elif self.test_type == 'mixed':
                    # 混合命令: 33% SET, 33% GET, 33% DEL
                    op_type = i % 3
                    if op_type == 0:
                        cmd = f"SET {key} {value}"
                    elif op_type == 1:
                        cmd = f"GET {key}"
                    else:
                        cmd = f"DEL {key}"

                cmd_bytes = cmd.encode()
                cmd_len = len(cmd_bytes)

                if self.use_binary_prefix:
                    # 二进制长度前缀 (4字节网络字节序)
                    prefix = struct.pack('>I', cmd_len)
                    sock.sendall(prefix + cmd_bytes)
                else:
                    # 文本长度前缀 (长度:内容)
                    prefix = f"{cmd_len}:".encode()
                    sock.sendall(prefix + cmd_bytes)

                # 读取响应
                try:
                    if self.use_binary_prefix:
                        # 读取4字节二进制长度前缀
                        length_data = b''
                        while len(length_data) < 4:
                            chunk = sock.recv(4 - len(length_data))
                            if not chunk:
                                raise ValueError("连接关闭，无法读取完整长度前缀")
                            length_data += chunk

                        resp_len = struct.unpack('>I', length_data)[0]
                    else:
                        # 读取文本长度前缀 (直到冒号)
                        length_data = b''
                        while b':' not in length_data:
                            chunk = sock.recv(1)
                            if not chunk:
                                raise ValueError("连接关闭，无法读取完整长度前缀")
                            length_data += chunk

                        # 解析长度
                        length_str = length_data.split(b':')[0].decode()
                        resp_len = int(length_str)

                    # 读取响应内容
                    resp_data = b''
                    while len(resp_data) < resp_len:
                        chunk = sock.recv(min(4096, resp_len - len(resp_data)))
                        if not chunk:
                            break
                        resp_data += chunk

                    # 任何响应都算成功
                    if resp_data:
                        success_count += 1

                except Exception as e:
                    print(f"线程 {thread_id} 读取响应错误: {str(e)}")
                    continue

                # 每批次操作后报告进度
                if (i + 1) % self.batch_size == 0 or i == self.ops_per_thread - 1:
                    now = time.time()
                    batch_elapsed = now - batch_start_time
                    total_elapsed = now - start_time
                    batch_ops_per_sec = self.batch_size / batch_elapsed if batch_elapsed > 0 else 0
                    total_ops_per_sec = (i + 1) / total_elapsed if total_elapsed > 0 else 0

                    print(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] 线程 {thread_id}: "
                          f"已完成 {i + 1}/{self.ops_per_thread} 操作, "
                          f"批次时间: {batch_elapsed:.3f}秒, 批次吞吐量: {batch_ops_per_sec:.2f} ops/sec, "
                          f"总体吞吐量: {total_ops_per_sec:.2f} ops/sec")

                    # 重置批次开始时间
                    batch_start_time = now

            end_time = time.time()
            elapsed = end_time - start_time

            self.results_queue.put({
                'thread_id': thread_id,
                'success': success_count,
                'time': elapsed,
                'ops_per_sec': self.ops_per_thread / elapsed if elapsed > 0 else 0
            })

        except Exception as e:
            print(f"线程 {thread_id} 错误: {str(e)}")
            self.results_queue.put({
                'thread_id': thread_id,
                'success': 0,
                'time': 0,
                'ops_per_sec': 0,
                'error': str(e)
            })
        finally:
            sock.close()

    def run_benchmark(self):
        """运行压力测试"""
        print(f"\n开始 {self.test_type.upper()} 压力测试:")
        print(f"- 主机: {self.host}:{self.port}")
        print(f"- 线程数: {self.threads}")
        print(f"- 每线程操作数: {self.ops_per_thread}")
        print(f"- 总操作数: {self.total_ops}")
        print(f"- 值大小: {self.value_size} 字节")
        print(f"- 协议格式: {'二进制长度前缀' if self.use_binary_prefix else '文本长度前缀'}")
        print("----------------------------------------")

        # 预热: 如果是GET或DEL测试，先设置一些键
        if self.test_type in ['get', 'del', 'mixed']:
            print("预热: 设置测试键...")
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            try:
                sock.connect((self.host, self.port))
                # 预生成固定大小的值
                base_value = "v" * self.value_size

                for t in range(self.threads):
                    for i in range(min(1000, self.ops_per_thread)):  # 最多预热1000个键
                        key = f"bench_key_{t}_{i}"
                        cmd = f"SET {key} {base_value}"
                        cmd_bytes = cmd.encode()
                        cmd_len = len(cmd_bytes)

                        if self.use_binary_prefix:
                            prefix = struct.pack('>I', cmd_len)
                            sock.sendall(prefix + cmd_bytes)
                        else:
                            prefix = f"{cmd_len}:".encode()
                            sock.sendall(prefix + cmd_bytes)

                        # 读取响应但不处理
                        try:
                            if self.use_binary_prefix:
                                length_data = sock.recv(4)
                                if len(length_data) == 4:
                                    resp_len = struct.unpack('>I', length_data)[0]
                                    sock.recv(resp_len)
                            else:
                                # 读取直到冒号
                                length_data = b''
                                while b':' not in length_data:
                                    chunk = sock.recv(1)
                                    if not chunk:
                                        break
                                    length_data += chunk

                                if b':' in length_data:
                                    length_str = length_data.split(b':')[0].decode()
                                    resp_len = int(length_str)
                                    # 已经读取的部分
                                    already_read = length_data[len(length_str)+1:]
                                    remaining = resp_len - len(already_read)
                                    if remaining > 0:
                                        sock.recv(remaining)
                        except Exception as e:
                            print(f"预热读取响应错误: {str(e)}")
                print("预热完成.")
            except Exception as e:
                print(f"预热错误: {str(e)}")
            finally:
                sock.close()

        threads = []
        start_time = time.time()

        # 创建并启动线程
        for i in range(self.threads):
            t = threading.Thread(target=self.worker, args=(i,))
            threads.append(t)
            t.start()

        # 等待所有线程完成
        for t in threads:
            t.join()

        end_time = time.time()
        total_elapsed = end_time - start_time

        # 收集所有线程的结果
        results = []
        while not self.results_queue.empty():
            results.append(self.results_queue.get())

        # 计算汇总指标
        total_success = sum(r['success'] for r in results)
        avg_thread_time = sum(r['time'] for r in results) / len(results) if results else 0
        total_ops_per_sec = self.total_ops / total_elapsed if total_elapsed > 0 else 0

        # 计算数据吞吐量
        bytes_per_sec = total_ops_per_sec * self.value_size
        mb_per_sec = bytes_per_sec / (1024 * 1024)

        # 打印结果
        print("\n----------------------------------------")
        print("压力测试结果:")
        print(f"- 测试类型: {self.test_type.upper()}")
        print(f"- 总操作数: {self.total_ops}")
        print(f"- 成功操作数: {total_success}")
        print(f"- 总耗时: {total_elapsed:.2f} 秒")
        print(f"- 平均线程耗时: {avg_thread_time:.2f} 秒")
        print(f"- 总吞吐量: {total_ops_per_sec:.2f} ops/sec")
        if self.value_size > 0:
            print(f"- 数据吞吐量: {mb_per_sec:.2f} MB/sec")
        print("----------------------------------------")

        # 打印每个线程的详细信息
        print("\n线程详情:")
        for r in sorted(results, key=lambda x: x['thread_id']):
            thread_id = r['thread_id']
            success = r['success']
            thread_time = r['time']
            ops_per_sec = r['ops_per_sec']

            print(f"线程 {thread_id}: 成功 {success}/{self.ops_per_thread} 操作, "
                  f"耗时 {thread_time:.2f} 秒, {ops_per_sec:.2f} ops/sec")

            if 'error' in r:
                print(f"  错误: {r['error']}")

        return {
            'test_type': self.test_type,
            'total_ops': self.total_ops,
            'successful_ops': total_success,
            'total_time': total_elapsed,
            'avg_thread_time': avg_thread_time,
            'total_ops_per_sec': total_ops_per_sec,
            'data_throughput_mb': mb_per_sec
        }


def main():
    parser = argparse.ArgumentParser(description='键值存储服务器压力测试工具')
    parser.add_argument('--host', default='localhost', help='服务器主机名')
    parser.add_argument('--port', type=int, default=7000, help='服务器端口')
    parser.add_argument('--threads', type=int, default=3, help='线程数')
    parser.add_argument('--ops', type=int, default=100000, help='总操作数')
    parser.add_argument('--batch', type=int, default=1000, help='进度报告批次大小')
    parser.add_argument('--value-size', type=int, default=100, help='值大小(字节)')
    parser.add_argument('--type', choices=['set', 'get', 'del', 'mixed', 'all'],
                        default='set', help='测试类型')
    parser.add_argument('--binary-prefix', action='store_true',
                        help='使用二进制长度前缀(4字节)而不是文本前缀')
    parser.add_argument('--simple', action='store_true',
                        help='简单模式: 更少的线程和操作，用于调试')

    args = parser.parse_args()

    # 简单模式覆盖设置
    if args.simple:
        args.threads = 1
        args.ops = 100
        args.batch = 10
        print("使用简单模式: 1线程, 100操作, 10批次大小")

    if args.type == 'all':
        # 运行所有类型的测试
        results = {}
        for test_type in ['set', 'get', 'del', 'mixed']:
            print(f"\n\n======= 开始 {test_type.upper()} 测试 =======\n")
            benchmark = KVBenchmark(
                host=args.host,
                port=args.port,
                threads=args.threads,
                total_ops=args.ops,
                batch_size=args.batch,
                value_size=args.value_size,
                test_type=test_type,
                use_binary_prefix=args.binary_prefix
            )
            results[test_type] = benchmark.run_benchmark()

        # 打印比较结果
        print("\n\n======= 测试结果比较 =======")
        print("操作类型      | 吞吐量 (ops/sec) | 数据吞吐量 (MB/sec) | 成功率 (%)")
        print("--------------+------------------+---------------------+-----------")
        for test_type in ['set', 'get', 'del', 'mixed']:
            r = results[test_type]
            throughput = r['total_ops_per_sec']
            data_throughput = r.get('data_throughput_mb', 0)
            success_rate = (r['successful_ops'] / r['total_ops'] * 100) if r['total_ops'] > 0 else 0
            print(f"{test_type.ljust(12)} | {throughput:,.2f} | {data_throughput:,.2f} | {success_rate:.2f}")
    else:
        # 只运行一种类型的测试
        benchmark = KVBenchmark(
            host=args.host,
            port=args.port,
            threads=args.threads,
            total_ops=args.ops,
            batch_size=args.batch,
            value_size=args.value_size,
            test_type=args.type,
            use_binary_prefix=args.binary_prefix
        )
        benchmark.run_benchmark()


if __name__ == "__main__":
    main()