#!/bin/bash
# ESP32/Arduino 플랫폼을 위한 개선된 빌드 스크립트

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
BUILD_DIR="${SRC_DIR}/build/arduino_fixed"
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

# 메인 스케치 파일 생성
(
    echo "#define MCP_OS_ARDUINO"
    echo "#define MCP_PLATFORM_ARDUINO"
    echo "#define MCP_CPP_FIXES"  # C++ 호환성 매크로 추가
    echo ""
    cat "$SRC_DIR/src/main.cpp"
) > "$ARDUINO_DIR/$SKETCH_NAME/$SKETCH_NAME.ino"

echo "Arduino 프로젝트 디렉토리 준비 중: $ARDUINO_DIR/$SKETCH_NAME"

# 필요한 헤더 파일 먼저 복사
echo "환경 제공 헤더 파일 생성 중..."

# 필요한 헤더 파일들 직접 복사
if [ -f "$SRC_DIR/src/system/logging.h" ]; then
    cp "$SRC_DIR/src/system/logging.h" "$ARDUINO_DIR/$SKETCH_NAME/logging.h"
    echo "logging.h 파일 복사됨"
fi

# MCP_Content 정의를 위한 중요 헤더 파일 생성
cat > "$ARDUINO_DIR/$SKETCH_NAME/arduino_compat.h" << 'EOL'
/**
 * @file arduino_compat.h
 * @brief Arduino 호환성을 위한 정의 파일
 * 
 * 이 파일은 Arduino 플랫폼에서 MCP 시스템이 작동하도록 하는
 * 다양한 호환성 정의를 제공합니다.
 */
#ifndef ARDUINO_COMPAT_H
#define ARDUINO_COMPAT_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// MCP_Content에 필요한 JSON 문자열 필드 추가
#define MCP_CONTENT_HAS_JSON_STRING 1

// MCP_Content 구조체 재정의 - Arduino 호환성을 위해
typedef struct MCP_Content {
    int type;                 // Content type 
    uint8_t* data;            // Content data
    size_t size;              // Content size
    char* mediaType;          // Media type string
    bool ownsData;            // Whether the structure owns the data
    const char* resultJson;   // Result as JSON string (Arduino 호환성)
} MCP_Content;

// Arduino용 시간 함수 
uint64_t MCP_GetCurrentTimeMs(void);

// 로깅 함수 - C++ 코드에서 직접 호출할 수 있도록
void log_error(const char* format, ...);
void log_warn(const char* format, ...);
void log_info(const char* format, ...);
void log_debug(const char* format, ...);
void log_trace(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif /* ARDUINO_COMPAT_H */
EOL
echo "arduino_compat.h 파일 생성됨"

# Arduino 플랫폼 전용 HAL 파일만 복사
echo "Arduino HAL 파일만 복사 중..."
if [ -d "$SRC_DIR/src/hal/arduino" ]; then
    # .c 파일을 .cpp로 변환하여 복사
    find "$SRC_DIR/src/hal/arduino" -name "*.c" | while read file; do
        base_name=$(basename "$file")
        cp "$file" "$ARDUINO_DIR/$SKETCH_NAME/${base_name%.c}.cpp"
        echo "Arduino HAL 파일 복사: ${base_name%.c}.cpp"
    done
    
    # .cpp 파일 복사
    find "$SRC_DIR/src/hal/arduino" -name "*.cpp" | while read file; do
        base_name=$(basename "$file")
        cp "$file" "$ARDUINO_DIR/$SKETCH_NAME/$base_name"
        echo "Arduino HAL 파일 복사: $base_name"
    done
    
    # .h 파일 복사
    find "$SRC_DIR/src/hal/arduino" -name "*.h" | while read file; do
        base_name=$(basename "$file")
        cp "$file" "$ARDUINO_DIR/$SKETCH_NAME/$base_name"
        echo "Arduino HAL 헤더 파일 복사: $base_name"
    done
else
    echo "경고: Arduino HAL 디렉토리를 찾을 수 없음: $SRC_DIR/src/hal/arduino"
fi

# 다른 디렉토리(hal 및 test 제외)에서 모든 파일 복사
echo "다른 소스 파일 복사 중 (비-Arduino HAL 및 테스트 파일 제외)..."

# JSON 파일을 헤더로 변환
echo "JSON 파일 관리 중..."
echo "JSON 파일을 헤더 파일로 변환 중..."

# config.json을 헤더로 변환
if [ -f "$SRC_DIR/src/core/mcp/config/config.json" ]; then
    cat > "$ARDUINO_DIR/$SKETCH_NAME/config_json.h" << 'EOL'
/**
 * @file config_json.h
 * @brief Default configuration JSON as a string constant
 */
#ifndef CONFIG_JSON_H
#define CONFIG_JSON_H

static const char* DEFAULT_CONFIG_JSON = 
EOL
    
    # 각 JSON 라인을 C 문자열로 변환
    sed 's/"/\\"/g' "$SRC_DIR/src/core/mcp/config/config.json" | sed 's/^/"/;s/$/\\n"/g' >> "$ARDUINO_DIR/$SKETCH_NAME/config_json.h"
    
    # 헤더 종료
    echo ";" >> "$ARDUINO_DIR/$SKETCH_NAME/config_json.h"
    echo "" >> "$ARDUINO_DIR/$SKETCH_NAME/config_json.h"
    echo "#endif /* CONFIG_JSON_H */" >> "$ARDUINO_DIR/$SKETCH_NAME/config_json.h"
    
    echo "config_json.h 파일 생성됨"
else
    echo "경고: config.json 파일을 찾을 수 없음: $SRC_DIR/src/core/mcp/config/config.json"
    
    # 기본 config_json.h 생성
    cat > "$ARDUINO_DIR/$SKETCH_NAME/config_json.h" << 'EOL'
/**
 * @file config_json.h
 * @brief Default configuration JSON as a string constant
 */
#ifndef CONFIG_JSON_H
#define CONFIG_JSON_H

static const char* DEFAULT_CONFIG_JSON = "{}";

#endif /* CONFIG_JSON_H */
EOL
    echo "빈 config_json.h 파일 생성됨"
fi

# mcp_time.h 생성
cat > "$ARDUINO_DIR/$SKETCH_NAME/mcp_time.h" << 'EOL'
/**
 * @file mcp_time.h
 * @brief Time utilities for MCP system
 */
#ifndef MCP_TIME_H
#define MCP_TIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get current time in milliseconds
 * 
 * @return uint64_t Current time in milliseconds
 */
uint64_t MCP_GetCurrentTimeMs(void);

#ifdef __cplusplus
}
#endif

#endif /* MCP_TIME_H */
EOL
echo "mcp_time.h 파일 생성됨"

# mcp_time_arduino.cpp 생성
cat > "$ARDUINO_DIR/$SKETCH_NAME/mcp_time_arduino.cpp" << 'EOL'
/**
 * @file mcp_time_arduino.cpp
 * @brief Time utilities for Arduino
 */

#include "mcp_time.h"
#include <time.h>

#if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get current time in milliseconds
 * 
 * @return uint64_t Current time in milliseconds
 */
uint64_t MCP_GetCurrentTimeMs(void) {
    // On Arduino, we can use millis() function which is available in Arduino.h
    // For this stub implementation, we'll use time(NULL) * 1000
    return (uint64_t)time(NULL) * 1000;
}

#ifdef __cplusplus
}
#endif

#endif // defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)
EOL
echo "mcp_time_arduino.cpp 파일 생성됨"

# logging_arduino.cpp 생성
cat > "$ARDUINO_DIR/$SKETCH_NAME/logging_arduino.cpp" << 'EOL'
/**
 * @file logging_arduino.cpp
 * @brief Arduino-specific logging implementation
 */
#include "logging.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#if defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)

// Arduino-specific logging functions
void log_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[ERROR] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void log_warn(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[WARN] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void log_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[INFO] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void log_debug(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[DEBUG] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void log_trace(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[TRACE] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

#endif // defined(MCP_PLATFORM_ARDUINO) || defined(MCP_OS_ARDUINO)
EOL
echo "logging_arduino.cpp 파일 생성됨"

# 소스 코드에서 include하는 필요한 다른 헤더 파일 복사
find "$SRC_DIR/src" -name "*.h" | while read header_file; do
    base_name=$(basename "$header_file")
    if [ ! -f "$ARDUINO_DIR/$SKETCH_NAME/$base_name" ]; then
        cp "$header_file" "$ARDUINO_DIR/$SKETCH_NAME/$base_name"
        echo "헤더 파일 복사: $base_name"
    fi
done

# .c 파일을 .cpp로 변환하여 복사
find "$SRC_DIR/src" \
    \( -path "$SRC_DIR/src/hal" -o -path "$SRC_DIR/src/test" \) -prune \
    -o -name "*.c" -print | while read file; do
    base_name=$(basename "$file")
    cp "$file" "$ARDUINO_DIR/$SKETCH_NAME/${base_name%.c}.cpp"
done

# .cpp 파일 복사
find "$SRC_DIR/src" \
    \( -path "$SRC_DIR/src/hal" -o -path "$SRC_DIR/src/test" \) -prune \
    -o -name "*.cpp" -print | while read file; do
    base_name=$(basename "$file")
    cp "$file" "$ARDUINO_DIR/$SKETCH_NAME/$base_name"
done

# ------------------------ 파일 내용 수정 ------------------------
echo "파일 내용 수정 중..."

# Arduino 호환성 헤더 포함
for cpp_file in "$ARDUINO_DIR/$SKETCH_NAME"/*.cpp; do
    if [ -f "$cpp_file" ]; then
        # arduino_compat.h 헤더 추가
        sed -i '' '1s/^/#include "arduino_compat.h"\n/' "$cpp_file"
    fi
done

# 모든 C/CPP 소스 파일에서 include 문 수정 - macOS 호환 방식
for file in "$ARDUINO_DIR/$SKETCH_NAME"/*.{cpp,h,ino}; do
    if [ -f "$file" ]; then
        echo "파일 처리 중: $(basename "$file")"
        
        # 단순하게 sed 명령 사용 - 라인별로 2단계 처리
        
        # 1. 정상적인 include 문 처리 (예: #include "path/to/header.h" -> #include "header.h")
        sed -i '' 's/#include[[:space:]]*"[^"]*\/\([^"]*\)"/#include "\1"/g' "$file"
        
        # 2. 불완전한 include 문 처리 (예: system/logging.h" -> #include "logging.h")
        sed -i '' 's/^\([[:space:]]*\)\([a-zA-Z0-9_]*\)\/\([a-zA-Z0-9_\.]*\)"/\1#include "\3"/g' "$file"
        
        # 3. operator 예약어 수정
        sed -i '' 's/\([^a-zA-Z0-9_]\)operator\([^a-zA-Z0-9_]\)/\1op\2/g' "$file"
        sed -i '' 's/^operator /op /g' "$file"
        sed -i '' 's/ operator;/ op;/g' "$file"
        sed -i '' 's/\.operator/\.op/g' "$file"
        sed -i '' 's/->operator/->op/g' "$file"
        
        # 4. config.json 포함 제거
        sed -i '' 's/#include "config.json"/#include "config_json.h"/g' "$file"
        
        # 5. MCP_ContentGetString 함수 호출 수정
        sed -i '' 's/MCP_ContentGetString(params, "moduleName", &moduleName)/MCP_ContentGetStringValue(params, "moduleName", \&moduleName)/g' "$file"
        sed -i '' 's/MCP_ContentGetString(params, "level", &levelStr)/MCP_ContentGetStringValue(params, "level", \&levelStr)/g' "$file"
        sed -i '' 's/MCP_ContentGetString(params, /MCP_ContentGetStringValue(params, /g' "$file"
        
        # 6. resultJson 필드 참조를 data로 변경
        sed -i '' 's/content->resultJson/MCP_ContentGetString(content)/g' "$file"
        sed -i '' 's/updatedContent->resultJson/MCP_ContentGetString(updatedContent)/g' "$file"
        
        # 7. time(NULL) 호출을 직접 변경
        sed -i '' 's/(double)time(NULL) \* 1000/(double)MCP_GetCurrentTimeMs()/g' "$file"
    fi
done

# ------------------------ 최종 검증 ------------------------
echo "파일 내용 검증 중..."

# 불완전한 include 문 검색
INCOMPLETE_INCLUDES=$(grep -l '[a-zA-Z0-9_]\+/[a-zA-Z0-9_\.]\+"' "$ARDUINO_DIR/$SKETCH_NAME"/* 2>/dev/null || echo "")
if [ -n "$INCOMPLETE_INCLUDES" ]; then
    echo "경고: 여전히 불완전한 include 문이 있습니다:"
    echo "$INCOMPLETE_INCLUDES"
    
    # 문제가 있는 파일 수동 수정
    for problem_file in $INCOMPLETE_INCLUDES; do
        echo "문제 파일 수정 중: $(basename "$problem_file")"
        
        # 문제가 있는 라인 표시
        problem_lines=$(grep -n '[a-zA-Z0-9_]\+/[a-zA-Z0-9_\.]\+"' "$problem_file" || echo "")
        echo "$problem_lines"
        
        # 각 문제 라인에 대해 직접 수정 - macOS 호환 버전
        echo "$problem_lines" | while IFS=: read -r line_num line_content; do
            # 정규표현식으로 헤더 이름 추출
            header_name=$(echo "$line_content" | grep -o '[a-zA-Z0-9_]\+\.[a-zA-Z0-9_\.]\+\"' | sed 's/\"//')
            if [ -n "$header_name" ]; then
                # 라인 수정
                echo "라인 $line_num 수정: $line_content -> #include \"$header_name\""
                sed -i '' "${line_num}s|.*|#include \"$header_name\"|" "$problem_file"
            fi
        done
    done
else
    echo "불완전한 include 문 문제가 해결됨"
fi

# operator 예약어 검색
OPERATOR_USES=$(grep -l '\boperator\b' "$ARDUINO_DIR/$SKETCH_NAME"/* 2>/dev/null || echo "")
if [ -n "$OPERATOR_USES" ]; then
    echo "경고: 여전히 operator 예약어가 사용되고 있습니다:"
    for op_file in $OPERATOR_USES; do
        echo "파일: $(basename "$op_file")"
        grep -n '\boperator\b' "$op_file"
        
        # 추가 수정 - 더 공격적인 operator 치환
        sed -i '' 's/operator/op/g' "$op_file"
    done
else
    echo "operator 예약어 문제가 해결됨"
fi

echo "평면 구조 테스트 스케치 생성 완료"

# Arduino 프로젝트 빌드
if command -v arduino-cli &> /dev/null; then
    # 모든 빌드용 공통 플래그 정의 - 플랫폼별 매크로 정의 추가
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
    COMPILE_CMD="$COMPILE_CMD \"${ARDUINO_DIR}/${SKETCH_NAME}\""
    
    echo "빌드 명령 실행: $COMPILE_CMD"
    
    # 명령 실행
    eval $COMPILE_CMD
    
    # 성공적으로 빌드되었으면 메시지 표시
    if [ $? -eq 0 ]; then
        echo "평면 구조 테스트 스케치 빌드 성공!"
        
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