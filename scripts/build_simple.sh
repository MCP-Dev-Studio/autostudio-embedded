#!/bin/bash
# ESP32/Arduino용 최소 빌드 스크립트 (bus_controllers 포함)

# 오류 발생 시 종료
set -e

usage() {
    echo "사용법: $0 [옵션]"
    echo "옵션:"
    echo "  --board=이름   - 사용할 보드 지정 (예: --board=esp32, --board=uno)"
    echo "  --port=경로    - 업로드용 포트 지정 (예: --port=/dev/ttyUSB0)"
    echo "  --help         - 이 도움말 표시"
    exit 1
}

# 소스 디렉토리
SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${SRC_DIR}/build/arduino_simple"
ARDUINO_DIR="${BUILD_DIR}/arduino_project"
SKETCH_NAME="MCP_Embedded"

# 기본 보드 및 포트
BOARD_FQBN="esp32:esp32:esp32"
PORT=""

# 인수 파싱
for arg in "$@"
do
    case $arg in
        --board=*)
        BOARD_NAME="${arg#*=}"
        
        # 다양한 보드 타입 처리
        case $BOARD_NAME in
            esp32)
                BOARD_FQBN="esp32:esp32:esp32"
                ;;
            uno)
                BOARD_FQBN="arduino:avr:uno"
                ;;
            nano)
                BOARD_FQBN="arduino:avr:nano"
                ;;
            mega)
                BOARD_FQBN="arduino:avr:mega"
                ;;
            nano33ble)
                BOARD_FQBN="arduino:mbed:nano33ble"
                ;;
            *)
                # 보드 이름을 FQBN으로 직접 사용
                BOARD_FQBN="$BOARD_NAME"
                ;;
        esac
        ;;
        --port=*)
        PORT="${arg#*=}"
        ;;
        --help)
        usage
        ;;
        *)
        echo "알 수 없는 옵션: $arg"
        usage
        ;;
    esac
done

echo "보드 FQBN: $BOARD_FQBN 사용"
if [ -n "$PORT" ]; then
    echo "포트: $PORT 사용"
fi

# 빌드 디렉토리 생성 및 청소
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
mkdir -p "${ARDUINO_DIR}/${SKETCH_NAME}"

# 필요한 디렉토리 생성
mkdir -p "${ARDUINO_DIR}/${SKETCH_NAME}/bus_controllers"

# 버스 컨트롤러 파일 복사
if [ -d "$SRC_DIR/src/core/device/bus_controllers" ]; then
    echo "버스 컨트롤러 파일 복사 중..."
    find "$SRC_DIR/src/core/device/bus_controllers" -name "*.h" | while read file; do
        base_name=$(basename "$file")
        cp "$file" "${ARDUINO_DIR}/${SKETCH_NAME}/bus_controllers/$base_name"
        echo "복사됨: ${base_name} -> bus_controllers/$base_name"
    done
fi

# 간단한 Arduino 스케치 생성
cat > "${ARDUINO_DIR}/${SKETCH_NAME}/${SKETCH_NAME}.ino" << 'EOL'
/**
 * MCP 임베디드 테스트 스케치 - 버스 컨트롤러
 */

// Platform defines
#define MCP_OS_ARDUINO
#define MCP_PLATFORM_ARDUINO
#define MCP_CPP_FIXES

// Arduino 코어 포함
#include <Arduino.h>

// 버스 컨트롤러 헤더 포함
#include "bus_controllers/gpio_controller.h"
#include "bus_controllers/i2c_controller.h"
#include "bus_controllers/spi_controller.h"

// 테스트용 LED 핀
const int ledPin = 2;  // ESP32 기본 내장 LED

// GPIO 설정
MCP_GPIOConfig gpioConfig = {
  MCP_GPIO_MODE_OUTPUT,  // 출력 모드
  MCP_GPIO_INT_NONE,     // 인터럽트 없음
  0,                     // 대체 기능 없음
  false                  // 초기 상태 LOW
};

void setup() {
  // 시리얼 초기화
  Serial.begin(115200);
  while (!Serial && millis() < 5000);
  
  Serial.println("\n\nMCP 버스 컨트롤러 테스트 시작...");
  
  // 일반 Arduino 방식으로 LED 설정
  pinMode(ledPin, OUTPUT);
  
  Serial.println("초기화 완료");
}

void loop() {
  // LED 깜빡임
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
  delay(500);
  
  // 상태 메시지
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 3000) {
    Serial.println("MCP 버스 컨트롤러 테스트 실행 중...");
    Serial.println("GPIO, I2C, SPI 컨트롤러 헤더 포함됨");
    lastPrint = millis();
  }
}
EOL

echo "버스 컨트롤러 테스트 스케치 생성 완료"

# Arduino 프로젝트 빌드
if command -v arduino-cli &> /dev/null; then
    # 모든 빌드용 공통 플래그 정의
    CPP_COMMON_FLAGS="-DMCP_PLATFORM_ARDUINO -DMCP_OS_ARDUINO -DMCP_CPP_FIXES"
    
    # 개선된 C++ 호환성을 위한 컴파일 명령 준비
    COMPILE_CMD="arduino-cli compile --verbose --fqbn \"$BOARD_FQBN\" \
        --build-property \"build.extra_flags=$CPP_COMMON_FLAGS\" \
        --build-property \"compiler.c.extra_flags=-w\" \
        --build-property \"compiler.cpp.extra_flags=-w -fpermissive -std=c++11 -Wno-error\""
    
    # 포트 지정되었으면 추가
    if [ -n "$PORT" ]; then
        COMPILE_CMD="$COMPILE_CMD --port \"$PORT\""
    fi
    
    # 대상 디렉토리 추가 - 메인 스케치 파일 컴파일
    COMPILE_CMD="$COMPILE_CMD \"${ARDUINO_DIR}/${SKETCH_NAME}/${SKETCH_NAME}.ino\""
    
    echo "빌드 명령 실행: $COMPILE_CMD"
    
    # 명령 실행
    eval $COMPILE_CMD
    
    # 성공적으로 빌드되었으면 메시지 표시
    if [ $? -eq 0 ]; then
        echo "버스 컨트롤러 테스트 스케치 빌드 성공!"
        
        # 포트 지정되었으면 업로드
        if [ -n "$PORT" ]; then
            echo "$PORT 포트로 보드에 업로드 중..."
            arduino-cli upload --fqbn "$BOARD_FQBN" --port "$PORT" "${ARDUINO_DIR}/${SKETCH_NAME}"
            
            if [ $? -eq 0 ]; then
                echo "업로드 성공!"
            else
                echo "업로드 실패."
            fi
        fi
    else
        echo "빌드 실패."
    fi
else
    echo "Arduino CLI를 찾을 수 없습니다. 먼저 설치하세요."
    echo "프로젝트가 ${ARDUINO_DIR}에 수동 빌드용으로 설정되었습니다"
fi