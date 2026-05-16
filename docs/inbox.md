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

### CI / Linux 资源约束

- [ ] **Linux runner 磁盘占用监控**：当前用 `jlumbroso/free-disk-space@main` 释放 ~28GB，给 Qt+OCCT 留余量。**未来如果引入更多大依赖**（boost / VTK / OpenCascade-Data 之类），可能再次撞 disk full
  - 上下文：W19 撞 `/usr/bin/ar: No space left on device` 在 libQt6Widgets.a 这步
  - 已上方案：workflow 开头清 .NET / Android SDK / Haskell / Docker
  - 候选升级：
    1. 加 `large-packages: true` 进一步清 apt cache（多腾 ~5GB，但 flaky）
    2. self-hosted runner 上有 100+ GB 可用
    3. `actions/runner@latest` 出过 large 版本（未稳定）
  - 风险：低（监控题）

- [ ] **VCPKG_MAX_CONCURRENCY=2 是不是仍必要**：一开始猜 OOM 加的，后来发现是 disk 问题。`MAX_CONCURRENCY=2` 让构建慢 ~+15 min。等磁盘问题确认解决后，可恢复默认（4）测一下，若不再 OOM 就保持默认
  - 风险：低（恢复后 monitor 1-2 个 sprint）

- [ ] **首次 cold build 太慢的备选**：如果 240/270 min timeout 仍不够（vcpkg 上游升级让 OCCT/Qt 编更久 / 加更多 port 后），考虑：
  1. **拆 workflow**：把 vcpkg install 拆成独立 job"warm cache"，后续 build/test job 依赖它（cache 命中后短）
  2. **vcpkg only-release**：自定义 triplet 设 `VCPKG_BUILD_TYPE=release`，让 vcpkg 只编 Release 而不编 Debug，省一半时间。代价：Debug 项目链接 Release vcpkg deps（C++ 多数情况下 OK）
  3. **跳过 OCCT 在 CI**：只在本地编 OCCT；CI 用 mock IGeometryPort。Phase 0 末再启用 OCCT CI。激进，但保 CI 在 30 min 内
  - 触发时机：当本周 push 之后 cold build 还是 timeout，再处理

### CI / vcpkg baseline 维护

- [ ] **Baseline 维护策略**：评估"runner 自带 vcpkg + git fetch + reset --hard"够不够稳定，还是需要切到"CI 自己 checkout vcpkg 到固定 commit"
  - 上下文：W19 ci.yml 撞两次 vcpkg baseline 错——第一次"baseline commit 不在 history"（光 fetch 解决），第二次"port version `7.9.3#1` 不在 versions DB"（fetch + `reset --hard origin/master` 解决）
  - 已上方案：ci.yml + sanitizers.yml 都加了 `sudo git fetch && sudo git reset --hard origin/master`（W19 末 push）
  - 风险：中。**`reset --hard` 把 runner vcpkg HEAD 推到 upstream/master 最新**——意味着 baseline 必须是 master 的 ancestor。如果 user 本地 vcpkg 切了 fork / 自定义分支，`vcpkg.json` 的 baseline 会与 CI 不匹配
  - 候选升级方案：当 push 到 GitHub 后第 1-2 个 sprint 仍偶发失败 → 切 self-checkout：
    ```yaml
    - uses: actions/checkout@v4
      with:
        repository: microsoft/vcpkg
        ref: <vcpkg.json's builtin-baseline>
        path: vcpkg
    - run: ./vcpkg/bootstrap-vcpkg.sh
    ```
    这样 CI 完全不依赖 runner image 的 vcpkg 状态。代价 +30s 每个 job + ~100MB 网络

### CI / 工作流（§3 决定不换工具，但优化项保留）

- [x] ~~**CI 切 vcpkg `x-gha` binary cache**（替代 actions/cache）~~  ← W19 走了一遍发现 x-gha 已被 vcpkg upstream 移除（2024+），**改用 actions/cache + baseline pin**（即 runner vcpkg 钉死到 vcpkg.json baseline）。真正修了"cache 永不命中"的根因（ABI 漂移），不是 cache 机制问题

- [ ] **未来切 NuGet to GitHub Packages 作为更可靠的 cache 后端**：actions/cache 有个已知问题——job timeout 时 post-step 不一定运行 → cache 可能没保存。NuGet 是 per-port 上传，每个 port 编完立即 push 到 GH Packages registry，不依赖 job 完成
  - 触发时机：cold build timeout 反复发生（应当不会，因为 baseline pin 后第二次开始就是 warm cache）
  - 复杂度：高（需 nuget.exe + mono on Linux + GITHUB_TOKEN 认证 + packages: write 权限）
  - 参考：[vcpkg + GH Packages 官方文档](https://learn.microsoft.com/vcpkg/consume/binary-caching-github-packages)

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

## 2026-05-16（Sprint 0.4 交付后录入）

### 渲染 / UI

- [ ] **GL context cleanup — 关闭窗口时 OpenGL 资源析构顺序错误**
  - 上下文：Sprint 0.4 交付后，关闭主窗口时触发异常/崩溃。`OpenGLRenderAdapter::~OpenGLRenderAdapter` 在 Qt GL context 已销毁后调用 `glDeleteVertexArrays` / `glDeleteBuffers` / `glDeleteProgram`，属于无效 GL 调用
  - 候选解：为 `OpenGLRenderAdapter` 增加 `cleanup()` 方法；在 `ViewportWidget::initializeGL()` 中把它连接到 `QOpenGLContext::aboutToBeDestroyed` 信号，保证 GL 资源在 context 销毁前被释放；destructor 改为只 reset PIMPL
  - 风险：低（仅影响退出时体验，不影响运行时功能；但若后续加 ASan 会报 use-after-destroy）
  - 计划：Sprint 0.5 T6 处理

---

## 模板（新加项参考此格式）

```markdown
- [ ] **简短标题** — 一行说明做什么
  - 上下文：为什么这件事冒出来 / 哪个 session 触发
  - 候选解：可选；已有方案
  - 风险：低 / 中 / 高（一句说明影响面）
```
