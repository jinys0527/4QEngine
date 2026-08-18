# 에셋 파이프라인 벤치마크 — FBX 직접 로드 vs `.meshbin`

원본 FBX를 런타임에서 Assimp로 직접 로드하는 경우와, 임포트 단계에서 변환해 둔
자체 바이너리 포맷(`.meshbin`)을 로드하는 경우의 비용을 비교한다.

측정일 : 2026-08-18

---

## 1. 결과

| 에셋 | FBX(KB) | bin(KB) | CPU(KB) | 정점 | 서브메시 | 스키닝 | FBX(ms) | bin(ms) | 배수 |
|---|---:|---:|---:|---:|---:|:---:|---:|---:|---:|
| Box                  |     14 |    1 |    1 |    24 |  1 | N |   0.30 | 0.013 | 22.4x |
| shinchan             |    254 |  330 |  445 |  4,960 |  9 | N |   7.31 | 0.124 | 59.0x |
| hiroshi              |    619 |  825 | 1,113 | 12,333 | 11 | N |  18.08 | 0.358 | 50.5x |
| cycle                |    804 | 1,202 | 1,625 | 18,115 |  8 | N |  26.46 | 0.711 | 37.2x |
| building             |    457 | 2,079 | 3,003 | 39,423 |  3 | N |  22.53 | 1.717 | 13.1x |
| all                  |  1,640 | 2,357 | 3,184 | 35,408 | 28 | N |  47.09 | 1.677 | 28.1x |
| Unarmed Walk Forward | 15,313 |  769 |  844 |  9,584 |  1 | Y |  27.71 | 0.335 | 82.7x |
| SKM_Quinn_Simple     |  3,928 | 5,759 | 6,292 | 68,279 |  6 | Y | 272.59 | 4.905 | 55.6x |
| **합계 (8종)**       |        |      |      | **188,126** | **67** | | **422.06** | **9.84** | **42.9x** |

- `CPU(KB)` : 로드 완료 시점의 정점 + 인덱스 버퍼 메모리 (`RenderData::Vertex` 기준)
- 배수 : `FBX(ms) / bin(ms)`

### 스켈레톤

| 에셋 | 본 수 | 상체 마스크 | 하체 마스크 |
|---|---:|---:|---:|
| SKM_Quinn_Simple     | 89 | 47 | 14 |
| Unarmed Walk Forward | 65 |  0 |  0 |

---

## 2. 해석

**로드 시간 — 42.9배**

FBX 직접 로드가 느린 이유는 파일 I/O가 아니라 Assimp가 매번 수행하는 후처리다.
삼각형화, 노멀 생성, 탄젠트 공간 계산, 중복 정점 병합, RH→LH 좌표계 변환은
모두 결과가 결정적(deterministic)이므로 실행할 때마다 반복할 이유가 없다.
임포트 단계에서 한 번 수행해 `.meshbin`으로 굳혀두면, 런타임이 하는 일은
헤더 검증 + 연속 바이트 읽기 + 정점 구조체 변환뿐이다.

**디스크 크기 — 약 1.4배 증가 (의도된 트레이드오프)**

`.meshbin`은 대부분의 에셋에서 원본 FBX보다 크다 (all: 1.6MB → 2.3MB).
FBX의 압축·인덱싱을 풀어 GPU 업로드 직전 레이아웃으로 펼쳐두기 때문이다.
디스크 1.4배를 내주고 로드 시간 42.9배를 얻는 교환이며, PC 타겟에서는 타당하다.

예외는 `Unarmed Walk Forward` (15.3MB → 0.77MB) 로, 원본 FBX에 애니메이션
트랙과 미사용 데이터가 함께 들어 있어 메시만 분리하면 오히려 크게 줄어든다.

**런타임 의존성 — 측정으로 확인**

`Client.exe`(Release, x64)의 import table을 `dumpbin -dependents`로 확인한 결과,
Assimp 및 그 전이 의존(`zlib1`, `pugixml`, `poly2tri`, `minizip`, `kubazip`, `draco`)이
**하나도 없다.** 서드파티 DLL은 `fmod.dll` 하나뿐이다.

| | Editor.exe | Client.exe |
|---|---|---|
| `assimp-vc143-mt(d).dll` | 의존 | **없음** |
| 그 전이 의존 6종 | 복사됨 | **없음** |
| 서드파티 DLL | assimp 계열 + fmod | **fmod 단독** |
| exe 크기 | 14.9MB (Debug) | 1.7MB (Release) |

Client 쪽 실제 의존 목록 :

```
USER32 / ole32 / KERNEL32 / d3d11 / D3DCOMPILER_47
MSVCP140 / VCRUNTIME140 / VCRUNTIME140_1 / api-ms-win-crt-*
fmod.dll
```

이를 위해 정리한 항목 (2026-08-18) :

- `Client/pch.h` — `#pragma comment(lib, "assimp-vc143-mt(d)")` 제거
- `Client/main.cpp` — 미사용 `#include "Importer.h"` 제거
- `Client.vcxproj` — `Importer` ProjectReference, vcpkg include/lib 경로 제거
- `Client.vcxproj` — PostBuild의 Assimp 계열 DLL 7종 복사 제거 (Debug / Debug_NoneBlur / Release)
- `Client/GameManager.h`, `Client/GameManager.cpp` — `Engine/GameManager.h`를 가리던
  stale 중복 헤더 제거 (vcxproj 미등록 orphan, 이것 때문에 Client가 빌드되지 않았다)

엔진 라이브러리 층(Engine / GameObject / ResourceManager / AI / Event / MRenderer)에는
원래부터 Assimp 참조가 없었다. 잔재는 `Client` 프로젝트 설정에만 남아 있었다.

> 남은 항목 : `Client`와 `Editor`가 출력 폴더 `$(SolutionDir)x64\`를 공유한다
> (`Client.vcxproj:168`, `Editor.vcxproj:114`). 따라서 Editor를 빌드하면 그 폴더에는
> Assimp DLL이 존재한다. **`Client.exe` 자체는 Assimp를 참조하지 않지만**,
> "배포 폴더에 Assimp가 없다"까지 말하려면 Client 전용 OutDir을 두거나
> 별도 패키징 단계가 필요하다. 현재 Client 실행에 필요한 것은
> `Client.exe` + `fmod.dll` + `Resources/` + `ResourceOutput/` + 컴파일된 `.cso` 뿐이다.

---

## 3. 측정 방법

측정 도구 : `Tools/assetbench/assetbench.cpp`

**FBX 경로**
- Assimp **Release** 빌드 (`assimp-vc143-mt.dll`, 5.4MB) 를 C API로 동적 로드
- post-process 플래그는 `Importer.cpp:593-601` 과 동일 (`0x0980002F`)
  - `Triangulate | GenNormals | CalcTangentSpace | GlobalScale | JoinIdenticalVertices | ConvertToLeftHanded`
- `aiImportFile` 호출만 계산. aiScene → 엔진 정점 변환, 임베디드 텍스처 추출,
  머티리얼/스켈레톤 직렬화는 **포함하지 않음** → 실제 격차는 이 수치보다 크다

**meshbin 경로**
- `AssetLoader.cpp:1262-1315` 의 읽기 경로를 그대로 재현
- 헤더 magic 검증 → 서브메시 → 인스턴스 트랜스폼 → 정점 → 인덱스 → 스트링 테이블
- `ToRenderVertex()` 변환 (본 가중치 정규화 포함) 까지 계산

**공통 조건**
- 벤치 도구 : MSVC 19.38, `/O2 /MT`, x64
- 워밍업 1회 후 **9회 측정 중앙값**, 파일 캐시 warm 상태
- GPU 버퍼 업로드는 양쪽 모두 제외 (동일 비용)
- 대상은 `ResourceOutput/` 에 존재하는 8개 에셋

## 4. 재현 방법

```bash
cl /nologo /O2 /MT /EHsc /std:c++20 /utf-8 Tools/assetbench/assetbench.cpp /Fe:assetbench.exe
```

레포 루트에서 실행한다 (에셋 경로가 상대 경로 기준).

```bash
assetbench.exe 9
```

인자는 반복 횟수(기본 7). Assimp DLL은 `VCPKG_ROOT`의 Release 빌드를 우선 사용하고,
없으면 `x64/assimp-vc143-mtd.dll`(Debug)로 폴백한다. Debug 빌드로 폴백하면
FBX 쪽 수치가 크게 부풀려지므로 결과 해석에 주의한다.
