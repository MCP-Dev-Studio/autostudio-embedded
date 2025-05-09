# Claude 작업 지침

## 중요 규칙

1. **스크립트에 코드 직접 삽입 금지**
   - 스크립트 내에 직접 코드를 삽입하는 방식(cat << EOL)으로 파일 생성 금지
   - 기존 소스 코드를 활용하여 플랫폼 및 보드별 분기 처리

2. **플랫폼 및 보드 유형 처리**
   - 스크립트에서는 단순히 컴파일 명령어와 옵션만 처리
   - 플랫폼 및 보드별 분기 로직은 소스 코드 내에서 처리

3. **유지보수 고려**
   - 코드 변경 시 스크립트도 함께 수정해야 하는 상황을 피할 것
   - 확장성을 고려한 설계 및 구현

4. **스크립트 기능**
   - 스크립트는 빌드/컴파일 과정만 담당
   - 소스 코드 변환 또는 수정은 최소화하고, 가능한 플랫폼별 분기 코드로 처리

## 잘못된 예시

```bash
# 코드를 스크립트에 직접 삽입 (이렇게 하지 말 것)
cat > "${MCP_EMBEDDED_DIR}/platform_compatibility.h" << 'EOL'
/**
 * @file platform_compatibility.h
 * ...
```

## 올바른 접근 방식

```bash
# 기존 파일을 복사하거나 기존 코드 활용
cp "${SRC_DIR}/src/platform/arduino/platform_compatibility.h" "${BUILD_DIR}/include/"

# 필요한 경우 간단한 텍스트 변환만 수행
sed -i 's/DEFINE_OLD/DEFINE_NEW/g' "${BUILD_DIR}/include/platform_compatibility.h"
```