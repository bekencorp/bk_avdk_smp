#!/usr/bin/env python3
"""
BK7259 4G ECM 下行速度测试自动化监控脚本

功能：
- 连接 BK7259 设备串口
- 自动执行 dltest 命令 20 次
- 解析并记录每次测速结果
- 生成测速报告

使用方法：
    python3 speed_test_monitor.py
"""

import serial
import time
import re
import sys
from datetime import datetime
from typing import List, Dict, Optional


# ==================== 配置参数 ====================
SERIAL_PORT = '/dev/cu.usbserial-120'  # 串口设备路径
BAUD_RATE = 115200                     # 运行时波特率
TEST_COUNT = 20                        # 测试次数
INTERVAL_SEC = 5                       # 每次测试间隔（秒）
TEST_DURATION_SEC = 10                 # 每次测试持续时间（秒）
WAIT_TIMEOUT_SEC = 15                  # 等待测试完成超时（秒）


class SpeedTestResult:
    """单次测速结果"""
    def __init__(self, test_id: int):
        self.test_id = test_id
        self.timestamp = datetime.now()
        self.success = False
        self.avg_kbps = 0
        self.total_bytes = 0
        self.error_msg = ""
        self.per_second_speeds: List[int] = []

    def __str__(self):
        if self.success:
            return (f"#{self.test_id} [{self.timestamp.strftime('%H:%M:%S')}] "
                   f"平均 {self.avg_kbps} Kbits/sec, 总共 {self.total_bytes} bytes")
        else:
            return f"#{self.test_id} [{self.timestamp.strftime('%H:%M:%S')}] 失败: {self.error_msg}"


class BK7259SpeedMonitor:
    """BK7259 测速监控器"""

    def __init__(self, port: str, baudrate: int):
        self.port = port
        self.baudrate = baudrate
        self.ser: Optional[serial.Serial] = None
        self.results: List[SpeedTestResult] = []

    def connect(self) -> bool:
        """连接串口"""
        try:
            self.ser = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=1
            )
            print(f"✓ 已连接到 {self.port}, 波特率 {self.baudrate}")
            time.sleep(0.5)
            # 清空缓冲区
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            return True
        except serial.SerialException as e:
            print(f"✗ 串口连接失败: {e}")
            return False

    def send_command(self, cmd: str) -> bool:
        """发送命令到设备"""
        try:
            self.ser.write((cmd + '\n').encode('utf-8'))
            return True
        except Exception as e:
            print(f"✗ 发送命令失败: {e}")
            return False

    def read_until_pattern(self, pattern: str, timeout_sec: float) -> List[str]:
        """读取串口数据直到匹配到模式或超时"""
        lines = []
        start_time = time.time()

        while time.time() - start_time < timeout_sec:
            if self.ser.in_waiting:
                try:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        lines.append(line)
                        print(f"  {line}")  # 实时打印

                        # 检查是否匹配结束模式
                        if re.search(pattern, line):
                            return lines
                except Exception as e:
                    print(f"读取出错: {e}")
            time.sleep(0.01)

        return lines

    def parse_result(self, lines: List[str], result: SpeedTestResult):
        """解析测试结果"""
        # 解析每秒速度：[0-1] sec 12345 Kbits/sec
        sec_pattern = r'\[(\d+)-(\d+)\]\s+sec\s+(\d+)\s+Kbits/sec'

        # 解析总结果：[0-10] sec avg 12345 Kbits/sec, 123456 bytes
        avg_pattern = r'\[0-(\d+)\]\s+sec\s+avg\s+(\d+)\s+Kbits/sec,\s+(\d+)\s+bytes'

        for line in lines:
            # 每秒速度
            match = re.search(sec_pattern, line)
            if match:
                kbps = int(match.group(3))
                result.per_second_speeds.append(kbps)

            # 平均速度和总字节数
            match = re.search(avg_pattern, line)
            if match:
                result.avg_kbps = int(match.group(2))
                result.total_bytes = int(match.group(3))
                result.success = True

            # 检测错误
            if 'failed' in line.lower() or 'error' in line.lower():
                result.error_msg = line
                result.success = False

    def run_single_test(self, test_id: int) -> SpeedTestResult:
        """执行单次测速"""
        result = SpeedTestResult(test_id)

        print(f"\n{'='*60}")
        print(f"第 {test_id}/{TEST_COUNT} 次测速 [{result.timestamp.strftime('%Y-%m-%d %H:%M:%S')}]")
        print(f"{'='*60}")

        # 发送 ap_cmd dltest 命令（BK7259 默认串口连接到 CP 核心，需要用 ap_cmd 前缀调用 AP 侧命令）
        if not self.send_command('ap_cmd dltest'):
            result.error_msg = "命令发送失败"
            return result

        # 读取测试输出（等待平均速率行）
        lines = self.read_until_pattern(
            r'\[0-\d+\]\s+sec\s+avg.*Kbits/sec',
            WAIT_TIMEOUT_SEC
        )

        if not lines:
            result.error_msg = "测试超时，未收到结果"
        else:
            self.parse_result(lines, result)

        self.results.append(result)
        return result

    def run_all_tests(self):
        """执行所有测速测试"""
        print(f"\n开始执行 {TEST_COUNT} 次测速测试...")
        print(f"每次测试间隔 {INTERVAL_SEC} 秒\n")

        for i in range(1, TEST_COUNT + 1):
            result = self.run_single_test(i)

            # 显示当前结果
            if result.success:
                print(f"\n✓ {result}")
            else:
                print(f"\n✗ {result}")

            # 最后一次不等待
            if i < TEST_COUNT:
                print(f"\n等待 {INTERVAL_SEC} 秒后进行下一次测试...\n")
                time.sleep(INTERVAL_SEC)

    def print_summary(self):
        """打印测试总结"""
        print(f"\n\n{'='*70}")
        print(f"测试总结报告 - {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print(f"{'='*70}\n")

        success_results = [r for r in self.results if r.success]
        failed_results = [r for r in self.results if not r.success]

        print(f"总测试次数: {len(self.results)}")
        print(f"成功: {len(success_results)} 次")
        print(f"失败: {len(failed_results)} 次")
        print(f"成功率: {len(success_results) / len(self.results) * 100:.1f}%\n")

        if success_results:
            speeds = [r.avg_kbps for r in success_results]
            bytes_list = [r.total_bytes for r in success_results]

            print(f"下行速度统计:")
            print(f"  最小值: {min(speeds)} Kbits/sec ({min(speeds) / 8:.2f} KB/s)")
            print(f"  最大值: {max(speeds)} Kbits/sec ({max(speeds) / 8:.2f} KB/s)")
            print(f"  平均值: {sum(speeds) / len(speeds):.0f} Kbits/sec ({sum(speeds) / len(speeds) / 8:.2f} KB/s)")
            print(f"\n总数据传输:")
            print(f"  最小值: {min(bytes_list)} bytes ({min(bytes_list) / 1024:.2f} KB)")
            print(f"  最大值: {max(bytes_list)} bytes ({max(bytes_list) / 1024:.2f} KB)")
            print(f"  平均值: {sum(bytes_list) / len(bytes_list):.0f} bytes ({sum(bytes_list) / len(bytes_list) / 1024:.2f} KB)")

        if failed_results:
            print(f"\n失败的测试:")
            for r in failed_results:
                print(f"  {r}")

        print(f"\n详细结果:")
        for r in self.results:
            print(f"  {r}")

    def save_report(self, filename: str):
        """保存测试报告到文件"""
        try:
            with open(filename, 'w', encoding='utf-8') as f:
                f.write(f"BK7259 4G ECM 下行速度测试报告\n")
                f.write(f"{'='*70}\n")
                f.write(f"测试时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
                f.write(f"串口: {self.port}\n")
                f.write(f"测试次数: {TEST_COUNT}\n")
                f.write(f"每次间隔: {INTERVAL_SEC} 秒\n")
                f.write(f"{'='*70}\n\n")

                success_results = [r for r in self.results if r.success]

                if success_results:
                    speeds = [r.avg_kbps for r in success_results]
                    f.write(f"成功次数: {len(success_results)}/{len(self.results)}\n")
                    f.write(f"平均速度: {sum(speeds) / len(speeds):.0f} Kbits/sec\n\n")

                f.write(f"详细结果:\n")
                f.write(f"{'-'*70}\n")
                for r in self.results:
                    f.write(f"{r}\n")

            print(f"\n✓ 报告已保存到: {filename}")
        except Exception as e:
            print(f"\n✗ 保存报告失败: {e}")

    def close(self):
        """关闭串口"""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print(f"\n串口已关闭")


def main():
    print("="*70)
    print("BK7259 4G ECM 下行速度自动化测试工具")
    print("="*70)
    print(f"串口: {SERIAL_PORT}")
    print(f"波特率: {BAUD_RATE}")
    print(f"测试次数: {TEST_COUNT}")
    print(f"测试间隔: {INTERVAL_SEC} 秒")
    print(f"每次测试时长: ~{TEST_DURATION_SEC} 秒")
    print("="*70)

    monitor = BK7259SpeedMonitor(SERIAL_PORT, BAUD_RATE)

    try:
        if not monitor.connect():
            return 1

        monitor.run_all_tests()
        monitor.print_summary()

        # 保存报告
        report_file = f"speed_test_report_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
        monitor.save_report(report_file)

    except KeyboardInterrupt:
        print("\n\n⚠ 用户中断测试")
    except Exception as e:
        print(f"\n✗ 测试出错: {e}")
        import traceback
        traceback.print_exc()
    finally:
        monitor.close()

    return 0


if __name__ == '__main__':
    sys.exit(main())
