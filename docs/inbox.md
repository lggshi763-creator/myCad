# Inbox — 未分类待办

> 跨文件、跨会话的临时记事。
>
> **生命周期**：每周日 review，逐项 promote / 关闭：
>   - **promote 到当周 devlog 末尾"待办"** —— 本 sprint / 下 sprint 内会做的
>   - **promote 到 GitHub Issue** —— 跨 sprint 持续追踪的
>   - **promote 到 ADR** —— 涉及不可逆决策的
>   - **关闭并加 strikethrough** —— 决定不做（写明理由）
>
> **AI 协作约定**（与 [CLAUDE.md](../CLAUDE.md) §AI 协作 ritual 一致）：
>   - AI session 启动时主动 Explore 本文件
>   - AI 任务完成后，自己有发现的 followup 主动加进来（不要让人手记）
>   - AI 改代码遇到 TODO 注释时主动评估能否当场解决

---

## 2026-05-06（W19 devlog 写作 + proposals 收尾时录入）

### 工具链 / 环境

- [ ] **vcpkg-reset-registry.ps1** — 沉淀"清 `$LOCALAPPDATA/vcpkg/registries/{git,git-trees}`"成脚本
  - 上下文：W19 复发 3 次 unborn-master 状态
  - 候选解：`tools/vcpkg-reset-registry.ps1` = `Remove-Item -Recurse -Force "$env:LOCALAPPDATA/vcpkg/registries/git*"` + `tasklist | grep TGitCache → kill`
  - 风险：清完会触发下次 cmake 时 1-3 min 的 registry re-clone（必须代理通）

- [ ] **VS Installer 卸 14.44 toolset**（长期清理）
  - 上下文：当前 preset pin 14.50 已兜底，不影响功能；但本机两个 toolset 共存是潜在的"以后再撞错位"风险
  - 候选解：VS Installer → 单个组件 → 搜 "14.44" → 取消勾选 → 修改
  - 风险：低（只是减组件不动主体）

- [ ] **GitHub push TLS error** `error:00000000:lib(0)::reason(0)`
  - 上下文：W19 §5 第 1 条，阻塞 V7 验证
  - 候选解：换代理节点 / 改 SSH 远端 / Gitee 镜像
  - 风险：中（如果长期不通会影响 CI 验证 + 团队协作）

### Preset / 命名一致性

- [ ] **preset 改名 `vs2022-*` → 实际版本**
  - 上下文：本机 VS 18 而非 VS 17，preset 名字误导
  - 候选解：批量重命名 + CI workflow 同步 + 文档同步 + build dir 改名（一次性 churn）
  - 风险：中（涉及 .github/workflows/ 多处 + README + devlog 引用）
  - 暂缓理由：本周已经够多变更，留到下次集中清理

### 文档 / 规约

- [ ] **Sprint 0.1 retrospective** `docs/retrospectives/sprint-0.1.md`
  - 上下文：roadmap §6.1 末尾的标准产物，等 V1-V8 全过后写
  - 候选解：参考本 W19 devlog §2 + §3 直接抽出来扩写

- [ ] **CMake preset 改名后 devlog 历史引用要 redirect**
  - 上下文：跟上面那条 preset 改名联动；devlog 里写过的"my-vs2022-debug" / "local-debug"等历史引用要保留还是要更新？
  - 决策：**保留历史**（devlog 是时间快照），但加 footer 说明"该 preset 后改名为 X"

### natvis / Sprint 0.2 衔接

- [ ] **解开 mycad.natvis 中预备的 Type entry**
  - 上下文：tools/visualizers/mycad.natvis 里 BRepHandle / EventId / Point2D/3D / `tl::expected` 都是注释化的，等对应类型在 Sprint 0.2 落地
  - 候选解：随每个值对象 PR 同步取消注释 + 调整字段名（MSVC STL 内部字段命名可能和样板有出入）
  - 风险：低（natvis 不影响构建，只是调试体验）

### CI / 工作流（§3 决定不换工具，但优化项保留）

- [ ] **CI 切 vcpkg `x-gha` binary cache**（替代 actions/cache）
  - 提案 §3.5 第 5 项；解压速度 2-3x
  - 候选解：[3.A] 提示词

- [ ] **CI 加 paths-filter 跳 docs-only 改动的构建矩阵**
  - 提案 §3.5 第 6 项；省 6 个 runner × N min
  - 候选解：[3.B] 提示词

- [ ] **dependabot 升 vcpkg.json baseline**
  - 提案 §3.5 第 8 项；每月一次 PR 减少手动同步
  - 候选解：[3.D] 提示词

### 代码内 TODO（[4.C] 扫描结果）

> 扫描时间：2026-05-06
> 范围：`*.cpp / *.hpp / *.h / *.cxx / *.hxx / *.cc / *.cmake`
> **结果：源码内零 TODO / FIXME / XXX / HACK 注释**。
> （项目尚处骨架阶段，代码量 < 500 行，业务逻辑还没开始写。）

后续随 Sprint 0.2+ 代码增长，pre-commit hook ([4.D]) 会在每次提交时
强制规范格式：`TODO(@<owner>, sprint-X.Y or issue-N): <text>`。
不规范的 TODO 会触发 advisory warning（不阻塞提交，仅提醒）。

---

## 模板（新加项参考此格式）

```markdown
- [ ] **简短标题** — 一行说明做什么
  - 上下文：为什么这件事冒出来 / 哪个 session 触发
  - 候选解：可选；已有方案
  - 风险：低 / 中 / 高（一句说明影响面）
```
