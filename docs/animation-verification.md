# 애니메이션 검증 — 상하체 분리 블렌딩 / 리타겟팅

`AnimationComponent`의 본 마스크 블렌딩과 리타겟 오프셋 계산이 실제로 올바르게
동작하는지, 렌더링 없이 `.skelbin` 데이터를 직접 읽어 검증한다.

검증일 : 2026-08-18
검증 도구 : `Tools/animverify/animverify.cpp`
재임포트 도구 : `Tools/reimport/reimport.cpp`

---

## 요약

| 항목 | 최초 검증 | 수정 후 |
|---|---|---|
| 블렌딩 수학 (`BlendLocalPoses`) | ✅ 4/4 | ✅ 4/4 |
| 리타겟 오프셋 수학 (동일 리그) | ✅ 오차 7.45e-08 | ✅ 동일 |
| 상하체 마스크 데이터 | ❌ 상체가 13조각으로 끊어짐 | ✅ **단일 서브트리, 누락 0** |
| Mixamo 리그 마스크 | ❌ 0/0 (분류 실패) | ✅ **54/12** |
| 리타겟 교차 리그 | ❌ 이름 일치 0/65 | ❌ **미해결 (설계 한계)** |
| 런타임 활성화 | ❌ 호출부 없음 | ❌ **미해결** |

**최종 : PASS 14 / FAIL 1.** 남은 1건은 교차 리그 리타겟으로, 별개의 설계 이슈다.

---

## A. 상하체 본 마스크 — 수정 완료

### 최초 문제

`SKM_Quinn_Simple`(89본)의 상체 마스크 47본이 계층상 **13조각**으로 끊어져 있었다.
정상이라면 루트는 `spine_01` 하나여야 한다.

```
루트 : spine_01, clavicle_l, clavicle_r, neck_01, head,
       index_01_l, middle_01_l, ring_01_l, pinky_01_l,
       index_01_r, middle_01_r, ring_01_r, pinky_01_r
```

원인은 `Importer/SkeletonBin.cpp`의 분류가 **UE 마네킹 본 이름을 하드코딩한 목록**이었고,
그 목록에 없는 본이 상체 체인 한가운데 끼어 있었기 때문이다.

| 끊긴 지점 | 목록에 없던 본 |
|---|---|
| 척추 → 목 | `spine_04`, `spine_05`, `neck_02` |
| 손 → 손가락 | `*_metacarpal_*` 8종 |
| 팔/다리 트위스트 | `*_twist_02_*` 8종 (`_twist_01`만 목록에 있었다) |

`defaultWeight=0`으로 쓰면 부모와 자식은 상체 애니메이션을 따라가는데 중간 본만
하체 포즈에 남아, 척추와 손가락 뿌리가 꺾여 보인다.

또한 이 목록은 UE 이름 전용이라 Mixamo 리그(`mixamorig:*`)에서는 한 개도 매칭되지
않아 마스크가 `0/0`으로 비어 있었다.

### 수정 : 이름 목록 → 계층 기반 분류

상체 루트 본을 이름으로 찾은 뒤 **그 서브트리를 통째로** 상체로 잡는다.
서브트리는 정의상 연결돼 있으므로 "중간 본 누락"이 구조적으로 불가능해진다.

- 상체 루트 후보 : `spine_01`, `mixamorig:Spine`, `Spine`, `spine`, `Spine1`, `Bip01_Spine1`
  (척추 체인의 최상위 본이 와야 하므로 `Spine`이 `Spine1`보다 앞에 온다)
- 하체 : 전체 − 상체 − 제외
- 제외 : `ik_*` 접두사, `interaction`, `center_of_mass` — 스키닝 변형에 관여하지 않는
  보조 본이므로 서브트리째 분류에서 뺀다
- 루트 본을 못 찾으면 기존 이름 목록 방식으로 폴백
- `SkelMeta/*.skelmeta.json`의 수동 지정은 기존대로 최우선

### 결과

```
SKM_Quinn_Simple : 91본, 상체 62, 하체 20, 미분류 9
  [PASS] 상체: 연결된 단일 서브트리 (루트 1개 = spine_01)
         하위로 새는 본 : 0개
  [PASS] 하체: 연결된 단일 서브트리 (루트 1개 = RootNode)
  [PASS] 상체 ∩ 하체 = 공집합
```

미분류 9본은 `ik_foot_root`/`ik_foot_l`/`ik_foot_r`/`ik_hand_root`/`ik_hand_gun`/
`ik_hand_l`/`ik_hand_r`/`interaction`/`center_of_mass` — 의도된 제외다.

Mixamo 리그(`Unarmed Walk Forward`)도 66본 중 상체 54 / 하체 12로 처음 분류됐다.

---

## B. 마스크 가중 블렌딩 — 정상 (수정 전후 동일)

```
[PASS] alpha=1, 상체마스크: 상체 본 62개가 상체 포즈를 100% 취함 (불일치 0)
[PASS] alpha=1, 상체마스크: 비상체 본 29개가 하체 포즈를 유지 (불일치 0)
[PASS] alpha=0.5: 상체 본이 정확히 중간 지점
[PASS] 마스크 없음: 전체 91본이 동일하게 블렌딩
```

`weightedAlpha = clamp(alpha * mask)` 구조가 의도대로 동작한다.
블렌딩 자체에는 처음부터 문제가 없었고, 문제는 A의 마스크 데이터였다.

---

## C. 리타겟팅

### C-1. 동일 스켈레톤 → 항등 오프셋 (정상)

```
[PASS] XMMatrixDecompose 실패 0건
[PASS] 항등 오프셋 (이탈 0본, 최대오차 T=7.45e-08 R=0.00e+00 S=1.19e-07)
```

`target × inverse(source)` 계산과 TRS 분해가 정확하다.

### C-2. 교차 리그 → 성립하지 않음 (미해결)

`SetRetargetFromSkeletonHandles`(`AnimationComponent.cpp:492`)는 두 스켈레톤을
**인덱스 순서로** 짝짓는다. 서로 다른 리그는 본 순서가 다르므로 가정이 깨진다.

```
SKM_Quinn_Simple(91본) <- Unarmed Walk Forward(66본)
[FAIL] 인덱스 i의 본 이름이 양쪽에서 일치 : 1/66
```

| i | target (Quinn) | source (Mixamo) |
|---:|---|---|
| 2 | `root` | `mixamorig:Spine` |
| 9 | `neck_01` | `mixamorig:LeftArm` |
| 11 | `head` | `mixamorig:LeftHand` |

decompose는 실패하지 않고(0건, 최대 translation 1.052) **조용히 잘못된 값을 만든다.**
이 구현이 유효한 범위는 **"본 순서가 동일하고 바인드 포즈만 다른 스켈레톤"** 이다.

해결하려면 인덱스가 아니라 **본 이름으로 매칭**해야 하며, 리그 간 이름이 다르면
(`spine_01` ↔ `mixamorig:Spine`) 이름 매핑 테이블이 필요하다. 별도 작업으로 남긴다.

---

## D. 런타임 활성화 여부 — 여전히 켜지지 않음

| API | 호출부 |
|---|---|
| `UseSkeletonUpperBodyMask` / `UseSkeletonLowerBodyMask` | 없음 |
| `SetRetargetFromBindPose` / `SetRetargetFromSkeletonHandles` | 없음 |

데이터 파이프라인(`SkeletonBin.cpp` → `.skelbin` → `AssetLoader` → `EnsureAutoBoneMask`)은
끝까지 이어져 있고 이제 데이터도 올바르다. **스위치를 켜는 코드만 없다.**
에디터는 `BoneMaskSource`를 읽기 전용으로 표시만 한다(`Editor/Util.cpp:2872`,
`REGISTER_PROPERTY_READONLY`).

---

## 부수 발견 : 임포터 드리프트

재임포트 과정에서 **현재 임포터 코드가 기존 커밋된 에셋을 재현하지 못한다**는 사실이 드러났다.

- 에셋 마지막 커밋 : `a23bcea` (2026-01-26)
- Importer 소스 마지막 변경 : `9566716` (2026-07-12) — 프로젝트 종료 후

`SkeletonBin.cpp`에 추가된 `CollectRequiredBoneNames`가 사용 본의 **모든 조상 노드**를
스켈레톤에 포함시키면서, Assimp의 FBX 피벗 헬퍼 노드
(`<Bone>_$AssimpFbx$_Translation / _PreRotation / _Rotation`)까지 본으로 들어왔다.

| 리그 | 커밋된 에셋 | 드리프트 상태 | 피벗 수정 후 |
|---|---:|---:|---:|
| SKM_Quinn_Simple | 89 | 91 | 91 |
| Unarmed Walk Forward | 65 | **198** | **66** |

`Importer.cpp`에 다음 한 줄을 추가해 해결했다.

```cpp
importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
```

늘어난 2본(`RootNode`, 메시 노드)은 조상 노드 포함 정책에 따른 정상 동작이다.

### 부수 효과 : 애니메이션 트랙 복원

피벗 노드가 애니메이션 채널을 쪼개고 있었다. 수정 후 트랙이 크게 늘었다.

| 클립 | 트랙 | 키 |
|---|---:|---:|
| `Unarmed Walk Forward` (수정 전) | 8 | 336 |
| `Unarmed Walk Forward` (수정 후) | **52** | **2,184** |

65본 Mixamo 워크 사이클에 8트랙은 비정상이었다.

### 회귀 없음 확인

재임포트로 실제 변경된 파일은 **9개뿐**이고, 스키닝이 없는 6개 에셋
(`Box`, `all`, `building`, `cycle`, `hiroshi`, `shinchan`)은 **바이트 단위로 동일**했다.
파이프라인이 결정적이며 이번 수정이 스키닝 에셋에만 영향을 준다는 뜻이다.

> `Dying`은 원본 FBX가 저장소에 없어 재임포트하지 못했다. 해당 클립은 4트랙 / 532키로
> 여전히 비정상으로 보이므로, 원본을 확보해 다시 임포트해야 한다.

---

## 재현 방법

```bash
cl /nologo /O2 /MT /EHsc /std:c++20 /utf-8 Tools/animverify/animverify.cpp /Fe:animverify.exe
```

레포 루트에서 실행한다. 실패 건수가 있으면 종료 코드 1.

```bash
animverify.exe
```

재임포트는 `Tools/reimport/reimport.cpp`를 빌드해 실행한다.
`<asset>/SkelMeta/<asset>.skelmeta.json`에 값이 있으면 자동 분류를 건너뛰므로,
자동 분류를 다시 돌리려면 해당 파일을 `{}` 로 비운 뒤 실행한다.
