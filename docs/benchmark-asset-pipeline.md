# 에셋 파이프라인 벤치마크 — FBX 직접 로드 vs `.meshbin`

원본 FBX를 런타임에서 Assimp로 직접 로드하는 경우와, 임포트 단계에서 변환해 둔
자체 바이너리 포맷(`.meshbin`)을 로드하는 경우의 비용을 비교한다.

측정일 : 2026-08-18
측정 도구 : `Tools/assetbench/assetbench.cpp`
대상 : 원본 FBX와 `.meshbin`이 모두 존재하는 **202개 에셋** (게임에 실제로 쓰이는 전체 세트)

---

## 1. 결과

| 항목 | FBX 직접 로드 | `.meshbin` | 비교 |
|---|---:|---:|---:|
| **로드 시간** | 765.2 ms | **21.86 ms** | **35.0배 빠름** |
| **파일 크기** | 49.5 MB | **32.1 MB** | **0.65배 (35% 작음)** |
| 런타임 정점+인덱스 메모리 | — | 41.2 MB | — |

정점 503,924개 / 서브메시 261개 / 스키닝 에셋 24개

### 배수 분포

| | 배수 |
|---|---:|
| 최소 | 7.5배 |
| 중앙값 | 25.6배 |
| 평균 | 27.9배 |
| 최대 | 79.0배 |

상위 : `Throw` 75.5배, `MainChar_ending` 59.7배, `vendingMachine` 53.5배,
`Cheetah_ending` 53.2배, `water` 52.8배

FBX 로드가 가장 오래 걸린 에셋 : `SKM_Quinn_Simple` 277.8 ms (정점 68,279개),
`all` 45.2 ms, `Throw` 28.2 ms

### 대표 에셋 발췌

| 에셋 | FBX(KB) | bin(KB) | 정점 | 스키닝 | FBX(ms) | bin(ms) | 배수 |
|---|---:|---:|---:|:---:|---:|---:|---:|
| Box | 14 | 1 | 24 | N | 0.28 | 0.014 | 20.2x |
| workLight | 33 | 58 | 1,100 | N | 0.82 | 0.034 | 24.5x |
| E1_combat_Ani | 133 | 187 | 2,405 | Y | 3.67 | 0.099 | 37.2x |
| water | 530 | 762 | 10,967 | N | 15.21 | 0.288 | 52.8x |
| SKM_Quinn_Simple | 3,928 | 5,759 | 68,279 | Y | 277.8 | 5.57 | 49.9x |

---

## 2. 해석

**로드 시간 — 35배**

FBX 직접 로드가 느린 이유는 파일 I/O가 아니라 Assimp가 매번 수행하는 후처리다.
삼각형화, 노멀 생성, 탄젠트 공간 계산, 중복 정점 병합, RH→LH 좌표계 변환은
모두 결과가 결정적(deterministic)이므로 실행할 때마다 반복할 이유가 없다.
임포트 단계에서 한 번 수행해 `.meshbin`으로 굳혀두면, 런타임이 하는 일은
헤더 검증 + 연속 바이트 읽기 + 정점 구조체 변환뿐이다.

**파일 크기 — 35% 감소**

`.meshbin`은 GPU 업로드 직전 레이아웃으로 정점을 펼쳐 저장하므로 압축·인덱싱이
풀린다. 그럼에도 전체적으로는 FBX보다 작다. 게임 에셋 대부분이 소품(정점
500~3,000개)이라, 메시 데이터보다 FBX가 안고 있는 부가 정보(노드 트리, 머티리얼
정의, 애니메이션 채널, 메타데이터)의 비중이 크기 때문이다.

> 표본에 따라 방향이 뒤집힌다. 고밀도 메시 8개만으로 측정했을 때는 `.meshbin`이
> 1.4배 더 컸다. 정점 6.8만 개짜리 `SKM_Quinn_Simple`은 여전히 1.47배 크다.
> **"크기가 줄어든다"가 아니라 "이 프로젝트의 에셋 구성에서는 줄어든다"가 정확한 서술이다.**

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
> 별도 패키징 단계가 필요하다.

**참고 : 로드된 리소스 규모**

`AssetLoader::LoadAll()`이 등록하는 리소스 (`Tools/assetprobe` 로 확인, 실패 0건) :

```
메시 204 / 머티리얼 207 / 텍스처 1,622
스켈레톤 24 / 애니메이션 23 / 셰이더 35(VS 15 + PS 20)
```

---

## 3. 측정 방법

**FBX 경로**
- Assimp **Release** 빌드 (`assimp-vc143-mt.dll`) 를 C API로 동적 로드
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
- 워밍업 1회 후 **5회 측정 중앙값**, 파일 캐시 warm 상태
- GPU 버퍼 업로드는 양쪽 모두 제외 (동일 비용)
- 원본 FBX는 `Resources/FBX`를 우선 탐색, 없으면 `MRenderer/fx`

## 4. 재현 방법

```bash
cl /nologo /O2 /MT /EHsc /std:c++20 /utf-8 Tools/assetbench/assetbench.cpp /Fe:assetbench.exe
```

레포 루트에서 실행한다 (에셋 경로가 상대 경로 기준).

```bash
assetbench.exe 5
```

인자는 반복 횟수(기본 7). `ResourceOutput/` 하위에서 `.meshbin`이 있고 원본 FBX도
찾을 수 있는 에셋을 전부 자동 수집한다. Assimp DLL은 `VCPKG_ROOT`의 Release 빌드를
우선 사용하고, 없으면 `x64/assimp-vc143-mtd.dll`(Debug)로 폴백한다. Debug 빌드로
폴백하면 FBX 쪽 수치가 크게 부풀려지므로 결과 해석에 주의한다.
