# 4QEngine — DirectX 11 자체 엔진 & 턴제 게임 (팀 프로젝트)

## 📌 프로젝트 개요

- 작업 기간 : 25.12.27 ~ 26.02.13 (약 7주)
- 프로젝트 유형 : 팀 프로젝트
- 제작 인원 : 4명 ✏️ *(실제 인원으로 수정)*
- 프로젝트 구성 : 자체 엔진(Engine) + 에디터(Editor) + 게임 클라이언트(Client)를 하나의 솔루션으로 개발
- 역할 :
    - **에셋 파이프라인** 설계 및 구현 (Assimp Importer → 자체 바이너리/JSON 포맷, 런타임 Assimp 의존 제거)
    - **스켈레탈 애니메이션 시스템** 설계 및 구현 (상태머신, 블렌딩 커브, 리타겟팅, 상하체 분리 블렌딩, GPU 스키닝 팔레트)
    - **UI 시스템** 설계 및 구현 (엔진 UI 컴포넌트 프레임워크 + UI 에디터 + 인게임 UI 전체)
    - **게임플레이 아키텍처** 설계 및 구현 (GameManager Phase/TurnState, 데이터 기반 FSM, Behavior Tree 프레임워크)
    - **엔진 코어** 구조 설계 (Service Locator, 이벤트 시스템, Serialize/Deserialize, 엔진–렌더러 분리 인터페이스)

## 🎥 플레이 데모 (GIF)

### 주사위 기반 턴제 전투

✏️ *(GIF 추가: 주사위 굴림 → 판정 → 전투)*

### 상하체 분리 블렌딩 애니메이션

✏️ *(GIF 추가: 이동하면서 공격하는 애니메이션)*

### UI 에디터

✏️ *(GIF 추가: 에디터에서 UI 배치 → 클라이언트에서 동일하게 렌더)*

## 🎮 구현 기능

- **Assimp 기반 에셋 임포터 + 자체 포맷 파이프라인 직접 설계 및 구현**
    - Editor/Importer에서만 Assimp로 FBX 로드 → Mesh는 `.meshbin`(magic/version 헤더 포함), Material/Skeleton/Animation은 JSON으로 직렬화
    - 런타임은 자체 포맷만 역직렬화 → Assimp 의존성 완전 제거
    - FBX 내장 텍스처 추출, PBR 텍스처 슬롯 자동 매핑, 스키닝 weight/index 저장, 서브메시 구조 지원
    - DX11 / LH / Row-major 좌표계 규약을 import 단계 한 곳에서 고정
- **스켈레탈 애니메이션 시스템 직접 설계 및 구현**
    - Playback(시간·루프·속도) → 클립 샘플링·TRS 보간 → Local Pose → Global Pose 누적 → `Global × inverseBindPose` 스키닝 팔레트 → GPU 업로드 전 과정 구현
    - 애니메이션 상태머신 : Enter/Update/Exit, 트랜지션 조건·우선순위, Linear/Ease 계열 블렌드 커브 선택
    - 본 마스크 기반 상하체 분리 블렌딩 (`UseSkeletonUpperBodyMask / LowerBodyMask`)
    - 바인드 포즈 차이 기반 애니메이션 리타겟팅 + 누락 본 처리 규칙
- **데이터 기반 FSM 시스템 설계 및 구현**
    - 상태/전이/액션을 JSON으로 저장하고, 런타임 공통 실행기가 등록된 Action/Event를 해석하는 구조
    - UI/Collision/Animation 이벤트를 문자열 이벤트로 매핑해 FSM 전이를 트리거, `Anim_BlendTo` 등 액션으로 애니메이션과 연결
    - 플레이어를 메인 FSM + 6개 서브 FSM(Combat/Move/Push/Inventory/Shop/Door)으로 분리
- **범용 Behavior Tree 프레임워크 직접 설계 및 구현**
    - Selector / Sequence / Parallel / Decorator / Task + Blackboard 구조
    - `Running` 시 active path 유지·재진입, Task 라이프사이클(OnEnter/OnTick/OnExit/OnAbort), Decorator Abort 일괄 처리
    - 트리 정의는 공유하고 실행 상태는 인스턴스별 메모리로 분리
    - 적 AI : 패트롤, 접근, 거리 유지, 도주 + 전투/비전투 트리 분리, 에디터 PatrolPoint 배치 저장
- **GameManager 기반 게임 흐름 제어**
    - Phase(GameStart → ExplorationLoop → CombatTrigger → TurnBasedCombat → Shop → NextFloor → GameOver)와 Exploration/Combat TurnState 이중 구조로 전체 루프 관리
    - Player/Enemy가 각자 넘기던 턴을 GameManager가 주관하도록 개선, 선제권 산출·턴 루프·연출 동기화
- **엔진 코어 구조**
    - Service Locator(`ServiceRegistry`) 기반 의존성 관리 — Engine/Editor/Client가 동일 컴포넌트 코드 공유
    - 씬 Serialize/Deserialize (Opaque/Transparent/UI 분리, 삭제된 오브젝트 정리 머지 로직)
    - 리소스는 파일에 key를 저장하고 로드 후 `ResourceStore`에서 handle로 resolve하는 참조 구조
    - 셰이더를 런타임 `.fx` 컴파일에서 빌드 타임 `.cso` 프리컴파일 로드로 개선
- **에디터 기능**
    - 컴포넌트 리플렉션 등록 및 프로퍼티 패널, ResourceBrowser, UI 에디터(복사/붙여넣기·다중 삭제)
    - CSV 파싱 기반 Loot/Shop/Combat 데이터 로딩, 언어별 로그 지원

## 🛠 기술 스택

- 엔진 : 자체 엔진 (DirectX 11 / LH / Row-major)
- 언어 : C++
- 기술 :
    - Component 기반 GameObject 시스템
    - 바이너리 에셋 포맷 설계 (magic/version 헤더, key 기반 리소스 참조)
    - 스켈레탈 애니메이션 (포즈 파이프라인 / 블렌딩 / 리타겟팅 / 본 마스크)
    - 데이터 기반 FSM / Behavior Tree
    - Service Locator 패턴, Event-driven 아키텍처
    - 셰이더 프리컴파일 파이프라인 (.hlsl → .cso)
- 라이브러리 :
    - Assimp (에디터 전용 임포트)
    - nlohmann::json (직렬화)
    - ImGui (에디터)
    - FMOD (사운드)

## 🧩 설계 포인트

### 1️⃣ 툴–런타임 분리

- Assimp는 Editor/Importer 프로젝트에만 링크, 런타임은 자체 포맷만 로드
- 공통 모듈은 Assimp 타입을 노출하지 않는 AssetFormat 계층으로 분리 → 런타임 의존성·로딩 비용·배포 복잡도 제거

### 2️⃣ 안정 참조(key)와 런타임 참조(handle)의 분리

- 실행마다 달라지는 `ResourceHandle(id, generation)`은 파일에 저장하지 않음
- 파일에는 key/path만 저장, Deserialize 후 `ResourceStore`에서 handle로 resolve

### 3️⃣ 코드와 데이터의 절충

- FSM의 핵심 로직(Action 구현)은 C++ Registry에 등록, 상태/전이/파라미터 조합은 JSON 데이터로 관리
- 그래프 에디터·범용 payload·Undo 같은 확장은 의도적으로 제외하고, 일정 내 실제로 돌아가는 편집/저장/로드 흐름을 우선

### 4️⃣ FSM과 BT의 역할 구분

- 상태와 전이가 명확한 영역(플레이어 행동, 애니메이션, 게임 Phase)은 FSM
- 조건·우선순위·Running·Abort가 중요한 적 AI는 BT — 두 구조를 억지로 통합하지 않음

### 5️⃣ 엔진–렌더러 경계 분리

- 씬이 Opaque/Transparent/UI로 나눠 RenderData를 생성해 넘기고 렌더러는 소비만 함
- 렌더러 담당 팀원과 충돌 없는 병렬 개발, 렌더러 교체에도 엔진 코드 무변경

### 6️⃣ Service Locator 기반 의존성 관리

- 명시적 DI의 생성자 파라미터 폭발 문제를 `ServiceRegistry`로 해결
- 미등록 서비스 접근 시 즉시 예외를 던져 "숨은 의존성" 단점을 조기 검출로 보완

## 🔎 핵심 설계

### 1️⃣ 에셋 파이프라인 (Assimp → 자체 포맷 → 런타임)

✏️ *(다이어그램 추가: FBX → Importer(Assimp) → .meshbin/.json → ResourceStore → GPU)*

**구조**

1. Editor/Importer에서 원본 파일(.fbx 등)을 Assimp로 로드
2. `aiScene/aiMesh/aiMaterial/aiAnimation`을 엔진 RenderData로 변환
3. Mesh는 `.meshbin`(헤더: magic, version, counts/offsets), Material/Skeleton/Animation은 JSON으로 직렬화
4. 런타임은 Assimp 없이 자체 포맷만 역직렬화 → `ResourceStore`가 key 기반 캐시(동일 key → 동일 handle) → GPU 리소스 생성

**설계 이유**

- Assimp를 런타임에 직접 붙이면 빠르지만 런타임 의존성·로딩 시간·메모리·배포 문제가 커짐
- 단순 라이브러리 연결이 아니라 에셋 파이프라인 전체 구조에 대한 결정으로 접근

**결과**

- 게임 실행 파일에서 외부 임포트 라이브러리 의존 제거
- 좌표계 변환을 import 단계 한 곳으로 고정해 런타임은 항상 엔진 기준 데이터만 취급

### 2️⃣ 스켈레탈 애니메이션 시스템

**구조**

- `AnimationComponent`(FSM·클립 재생·블렌딩·리타겟팅으로 Local Pose 생성)와 `SkeletalMeshComponent`(Global Pose 누적·스키닝 팔레트 변환) 책임 분리
- 파이프라인 : Playback(시간·루프·normalized time) → 클립 샘플링·TRS 보간 → Local Pose → 계층 누적 Global Pose → `Global × inverseBindPose` → GPU 팔레트 업로드

**주요 기능**

- 상태머신 : Enter/Update/Exit, 전이 조건·우선순위, `StartBlend` 기반 크로스 블렌딩(Linear/EaseIn/EaseOut 커브 선택)
- 상하체 분리 : 본별 weight 마스크로 BasePose/UpperPose 합성 — 하체는 이동, 상체는 공격 동시 재생
- 리타겟팅 : source/target 스켈레톤의 바인드 포즈 차이로 오프셋 계산, 누락 본 처리 규칙 포함

**결과**

- 이동 중 공격 등 상·하체 독립 연출
- 서로 다른 리그의 애니메이션 에셋을 하나의 캐릭터에서 재사용

### 3️⃣ 데이터 기반 FSM + Player FSM

**구조**

- 상태/전이/액션을 JSON으로 직렬화하는 FSMGraph, 에디터 Inspector에서 편집
- Action/Event는 C++ Registry에 등록된 것만 에디터에서 선택 가능 — 핵심 로직은 코드, 조합은 데이터
- UI/Collision/Animation 이벤트를 문자열 이벤트로 매핑해 전이 트리거

**Player FSM 구성**

- 메인 `PlayerFSMComponent`(Idle/Move/Shop/Combat/Inventory/Push/Door)가 허브 역할
- 서브 FSM 6개로 기능 분리 : Combat(RangeCheck→CostCheck→Confirm→턴 진행), Move, Push, Inventory, Shop(ItemSelect→SpaceCheck→MoneyCheck→Buy), Door
- UI/입력이 먼저 플래그를 세팅하고, FSM 액션 핸들러가 `Consume*` 계열 플래그를 소비 후 기본값 복구하는 규칙으로 입력–상태 전이 동기화

**결과**

- 상태 전이를 코드 수정 없이 데이터로 조정 가능
- 기능별 서브 FSM 분리로 상태 폭발 없이 복잡한 플레이어 행동 관리

### 4️⃣ Behavior Tree 프레임워크

**구조**

- 모든 노드는 `Tick(ctx)`로 실행, `Success/Failure/Running` 반환
- `Running`이면 active path를 유지하고 다음 Tick에 같은 노드부터 재진입
- Task 라이프사이클 : OnEnter / OnTick / OnExit(status) / OnAbort, Decorator Abort는 Tick 시작 시점 일괄 처리
- Blackboard는 key별 version 관리(값 변경 시에만 증가), 트리 정의는 공유하되 실행 상태는 인스턴스별 `memory[nodeId]`로 분리

**게임 적용**

- Blackboard 키 : TargetActor, TargetDistance, TargetInSight, IsInCombat, IsMyTurn, CurrentHP 등
- Task : MoveToTarget, RotateToTarget, StartAttack, ApplyAttackRoll, ApplyDamage, EndTurn
- 전투/비전투 트리 분리, 패트롤·접근·거리 유지·도주 구현, 에디터 PatrolPoint 배치 저장

**결과**

- 적 행동을 노드 조합과 에디터 데이터만으로 구성
- 하나의 트리 정의를 여러 적 인스턴스가 공유해 메모리·관리 비용 절감

### 5️⃣ GameManager Phase / TurnState 게임 흐름

**구조**

- `Phase` : GameStart → InitCharacter → ExplorationLoop → CombatTrigger → CombatInit → TurnBasedCombat → CombatEnd → Shop → NextFloor → GameOver
- 각 Phase의 onEnter/onExit에서 수위(Flood) 활성화, Player FSM 이벤트 디스패치, CombatTurnState 초기화, 층 전환 요청 수행
- `ExplorationTurnState`(PlayerTurn ↔ EnemyStep)와 `CombatTurnState`(SelectActor → PlayerTurn/EnemyTurn → Resolve 순환) 이중 턴 구조

**초기 문제**

- Player/Enemy가 각자 턴을 넘기는 구조라 전투 난입·스테이지 전환 시 턴 루프가 꼬임

**개선**

- 턴 진행 권한을 GameManager로 일원화, FSM·BT·UI는 이벤트로만 통신

**결과**

- 전투 난입, 층 전환 등 흐름 변화에도 턴 루프 일관성 유지
- 주사위 연출이 끝날 때까지 턴을 대기시키는 등 연출–로직 동기화

### 6️⃣ 컴포넌트 기반 UI 시스템 + UI 에디터

**구조**

- Button / Image(Tint) / Text / Slider / ProgressBar / NumberSprite / DicePanel 등 UI 컴포넌트 프레임워크
- UIFSMComponent로 상점·문·ESC 메뉴 등 UI 상태 전환 관리
- 에디터에서 배치·저장한 UI가 클라이언트에서 동일하게 렌더

**렌더링 확장**

- ProgressBar FillDirection + Arc(원형 게이지) 전용 픽셀 셰이더, 마스크 이미지, 알파 클립
- HorizontalBox 레이아웃 자동 적용, 부모 기준 Offset 바운드 계산

**결과**

- 주사위 연출, 선제권 표시, 상점/자판기, 스테이지 UI 등 인게임 UI 전체를 이 프레임워크로 구현
- UI 배치·수정을 에디터에서 수행 → 코드 수정 없이 반복 개선

## ⚠ 트러블슈팅

### ⭐ 바이너리 직렬화 시 컨테이너 덤프 문제

- 문제 : Mesh 데이터를 바이너리로 저장·로드하면 데이터가 깨지거나 로드 시 크래시 발생
- 원인 : `std::vector`, `std::string`을 구조체째 덤프하면 내부 포인터가 함께 저장되어, 로드 시 무효 포인터를 읽게 됨
- 해결 1 : 포맷을 magic, version, count, offset, raw bytes, string(length + bytes)로 명시적으로 구성
- 해결 2 : Deserialize 시 파일 크기 대비 expected size 검증, 비정상적으로 큰 count 차단, 버전 불일치 검사 등 방어 코드 추가
- 결과 : 저장/로드 안정성 확보, 포맷 변경 시에도 version 검사로 하위 호환 문제를 조기 검출

### ⭐ 리소스 Handle을 파일에 저장하던 문제

- 문제 : 씬을 저장 후 다시 로드하면 컴포넌트가 엉뚱한 리소스를 참조하거나 참조가 끊어짐
- 원인 : `ResourceHandle(id, generation)`은 실행마다 값이 달라지는 런타임 식별자인데, 이를 파일에 그대로 직렬화함
- 해결 1 : 파일에는 key/path 같은 안정적인 참조만 저장
- 해결 2 : Deserialize 이후 resolve 단계에서 `ResourceStore`가 key를 handle로 복원 (동일 key → 동일 handle 캐시)
- 해결 3 : Mesh의 material 참조, Material의 texture 참조, Scene/Component의 리소스 참조에 동일 원칙 적용
- 결과 : 실행 세션과 무관하게 재현 가능한 저장 파일 확보, 리소스 참조 일관성 확보

### 좌표계 / 행렬 변환 이슈

- 문제 : Assimp로 임포트한 모델이 뒤집히거나, 노멀맵·컬링이 어긋나고, 본 트랜스폼이 이중 적용됨
- 원인 1 : Assimp(RH) → DX11(LH/Row-major) 변환 시 position만 변환하고 normal, tangent, winding/culling을 함께 맞추지 않음
- 원인 2 : `aiMatrix4x4` → `XMFLOAT4X4`를 memcpy로 옮기면서 행/열 순서가 어긋남
- 원인 3 : 노드 트리의 local/global transform 누적이 중복 적용되는 경로 존재
- 해결 1 : 행렬은 원소를 명시적으로 옮기고, RH→LH 변환 시 normal/tangent/winding까지 일괄 처리
- 해결 2 : 좌표계 변환을 import 단계 한 곳에서만 수행하도록 규약 고정 — 런타임은 엔진 기준 데이터만 취급
- 해결 3 : 이동만 있는 큐브, Yaw 90° 큐브, 노멀맵 평면으로 검증 케이스 구성
- 결과 : 임포트 결과의 방향·스키닝 정확성 확보, 이후 에셋 추가 시 변환 문제 재발 없음

### Runtime에 Assimp 의존성이 전파되는 문제

- 문제 : `vcpkg integrate install` 사용 시 VS 솔루션 전체에 vcpkg include/lib 경로가 노출되어, 런타임 프로젝트가 실수로 Assimp를 include/link할 위험
- 원인 : 전역 통합 방식이 프로젝트별 의존성 경계를 무너뜨림
- 해결 1 : Importer/Editor와 Game/Runtime을 프로젝트 수준에서 분리, Assimp는 Importer에만 링크
- 해결 2 : 공통 모듈은 Assimp 타입을 노출하지 않는 AssetFormat 계층(직렬화 구조체와 헤더만 공유)으로 분리
- 결과 : 게임 런타임에서 Assimp include 자체가 불가능한 구조로 의존성 격리

### UI 이벤트 증발 이슈

- 문제 : 프레임(테두리) 이미지를 UI 위에 씌우면 그 아래 버튼들이 클릭 이벤트를 받지 못함
- 원인 : 최상위 UI가 이벤트를 가로채고 하위로 전달하지 않는 구조
- 해결 : 클릭 좌표 기준으로 해당 위치의 모든 UI 오브젝트에 이벤트를 전달하는 위치 기반 라우팅으로 변경, 각 컴포넌트가 자신이 처리할 이벤트인지 판단
- 결과 : 장식용 이미지와 상호작용 UI를 자유롭게 겹쳐 배치 가능

### 상태 전환 중 오브젝트 삭제 크래시

- 문제 : 턴/FSM 상태 전환 도중 오브젝트를 즉시 삭제하면 순회 중인 컨테이너가 무효화되어 크래시 발생
- 원인 : 이벤트 처리 중 씬 구조를 즉시 변경하는 재진입 문제
- 해결 : 삭제 요청을 `m_PendingRemovalNames`에 쌓고 프레임 끝에 `ProcessPendingRemovals()`로 일괄 처리하는 지연(deferred) 방식으로 변경
- 결과 : 상태 전환·이벤트 처리 중 크래시 제거

## 📈 성과

- 7주 내 **자체 엔진 + 에디터 + 플레이 가능한 턴제 게임** 완성 (팀 4명 ✏️ *(인원 수정)*)
- 전체 커밋의 약 **31% (180/575, 머지 제외)** 기여 — Importer, 애니메이션, AI(BT), Event, UI 모듈 주도
- Assimp를 툴에 격리하고 자체 포맷 파이프라인을 구축해 **런타임 외부 의존성 제거**
- key 기반 리소스 참조와 방어적 바이너리 포맷으로 **저장/로드 재현성 확보**
- 포즈 파이프라인부터 GPU 팔레트까지 **스켈레탈 애니메이션 전 과정 직접 구현**, 상하체 블렌딩·리타겟팅으로 에셋 재사용성 확보
- 데이터 기반 FSM + BT 프레임워크로 **플레이어 행동과 적 AI를 코드 수정 없이 데이터로 구성**
- GameManager Phase/TurnState 일원화로 **전투 난입·층 전환에도 턴 루프 일관성 유지**
- RenderData 인터페이스 분리로 **렌더러 담당 팀원과 충돌 없는 병렬 개발** 실현
- UI 에디터 구축으로 **코드 수정 없이 UI 배치·반복 개선** 가능

## 📚 프로젝트 회고

### ✅ 잘한 점

- 구현 전에 조사 문서(엔진 구조, Assimp, 애니메이션, BT)를 만들고 체크리스트로 완료 여부를 추적하는 방식으로 작업함 — 좌표계·직렬화 같은 위험 요소를 사전에 식별
- Assimp 런타임 포함 여부, 텍스트/바이너리 포맷, FSM 코드/데이터화 같은 갈림길마다 트레이드오프를 문서로 정리하고 결정함
- 그래프 에디터·Undo·범용 payload 같은 확장 욕심을 의도적으로 잘라내고, 일정 내 실제로 돌아가는 편집/저장/로드 흐름을 우선함
- 이전 프로젝트에서 아쉬웠던 이벤트 기반 구조 전환을 실제로 적용해 전투/UI/게임 흐름을 이벤트로 분리함

### ⚠ 아쉬운 점

- 일정 압박으로 UI 해상도 대응을 포기하고 고정 해상도(2560×1600)로 제한함
- 텍스처 로더·리임포트 정책·pack 단계 등 파이프라인 후반 항목을 계획만 하고 완료하지 못함
- 상태 전환(FSM/턴 루프) 버그를 후반에 몰아서 잡음 — 상태 다이어그램을 먼저 확정했다면 디버깅 비용이 줄었을 것
- 커밋 메시지 품질 관리가 안 된 구간이 있어 이력 추적 비용이 커짐

### 🚀 개인 성장 포인트

- 엔진 코어 → 에디터 툴링 → 게임플레이까지 **전 층위를 관통하는 구조 설계** 경험 확보
- "런타임에 무엇을 남기고 무엇을 툴로 뺄 것인가"라는 **파이프라인 관점의 구조 결정** 경험
- 디자인 패턴(Service Locator, Event, BT, FSM)을 교과서가 아닌 **실제 문제(머지 충돌, 이벤트 증발, 재진입 크래시)를 겪고 선택**하는 경험
- 좌표계·행렬·바이너리 직렬화·리소스 핸들 등 **저수준 구현 함정을 원인과 해결까지 문서화**하는 습관 확립

## 🏷 장르 / 플랫폼

- 장르 : 턴제 던전 탈출 (주사위 기반 전투) ✏️ *(게임 정식 장르로 수정)*
- 플랫폼 : PC (Windows, DirectX 11)

## 🔗 링크

- GitHub : https://github.com/jinys0527/4QEngine
- 작업 기록 (Notion) : https://app.notion.com/p/39a8c73c43f980e9867aed98a688d9d4
- 영상 : ✏️ *(플레이 영상 링크)*
