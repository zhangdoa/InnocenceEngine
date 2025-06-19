# RenderingClient Pass Development Guidelines

## Core Patterns from OpaquePass

### Update() Pattern
```cpp
bool YourPass::Update()
{
    // ALWAYS check activation status first
    if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
        return false;
    
    if (m_criticalResource->m_ObjectStatus != ObjectStatus::Activated)
        return false;
    
    // Then proceed with logic
    auto l_renderingServer = g_Engine->getRenderingServer();
    // ... rest of update logic
}
```

### PrepareCommandList vs ExecuteCommands Separation
**CRITICAL ARCHITECTURAL PATTERN:**

- **PrepareCommandList()** - Command list RECORDING:
  - `CommandListBegin()` / `CommandListEnd()`
  - `BindRenderPassComponent()` / `BindGPUResource()`
  - `Dispatch()` for compute passes
  - **`ExecuteIndirect()` MUST be here** - it records commands, doesn't execute them

- **DefaultRenderingClient::ExecuteCommands()** - Command list SUBMISSION:
  - `ExecuteCommandList()` submits completed command lists to GPU
  - GPU synchronization (`WaitOnGPU`, `SignalOnGPU`)
  - **NO command recording here** - command lists are already closed

```cpp
// CORRECT: ExecuteIndirect in PrepareCommandList (command recording)
bool SunShadowGeometryProcessPass::PrepareCommandList(IRenderingContext* renderingContext)
{
    l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
    // ... bind resources ...
    
    // Record indirect draw command while command list is open
    auto l_indirectDrawCommandBuffer = reinterpret_cast<GPUBufferComponent*>(SunShadowCullingPass::Get().GetResult());
    l_renderingServer->ExecuteIndirect(m_RenderPassComp, l_indirectDrawCommandBuffer);
    
    l_renderingServer->CommandListEnd(m_RenderPassComp); // Close command list
}

// CORRECT: ExecuteCommandList in ExecuteCommands (command submission)
if (SunShadowGeometryProcessPass::Get().GetStatus() == ObjectStatus::Activated)
{
    l_renderingServer->ExecuteCommandList(SunShadowGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
    l_renderingServer->SignalOnGPU(SunShadowGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
}
```

**KEY INSIGHT:** `ExecuteIndirect()` doesn't execute anything immediately - it records an indirect draw command into the command list. The actual execution happens when the GPU processes the submitted command list.

### Resource Binding Consistency
- **Setup()** descriptor indices MUST match **PrepareCommandList()** binding indices
- Comment bindings clearly: `// b0, u1, t2` etc.
- Validate binding order matches shader expectations

### Required Methods
```cpp
bool Setup(ISystemConfig* systemConfig);
bool Initialize();
bool Update();                           // Early activation checks
bool Terminate();
ObjectStatus GetStatus();
bool PrepareCommandList(IRenderingContext*); // Prepare commands, NO execution
RenderPassComponent* GetRenderPassComp();
```

### Testing and Validation
**CRITICAL: Use Main.exe for real rendering validation, NOT Test.exe**

- **Test.exe**: Headless mode, no rendering server initialization
- **Main.exe**: Full rendering pipeline with DX12/Vulkan

**Proper test command:**
```bash
# Working directory: C:\GitRepo\InnocenceEngine\Bin
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Main.exe -mode 0 -renderer 0 -loglevel 0" 2>&1
```

**Test args (from .vscode/launch.json):**
- `-mode 0`: Windowed mode
- `-renderer 0`: DX12 renderer  
- `-loglevel 0`: Verbose logging

**Validation Methods for Graphics Changes (without visual confirmation):**

1. **Log Analysis**:
   ```bash
   # Check latest log for errors
   grep -i "error\|warning\|d3d12.*error\|validation.*failed" /mnt/c/GitRepo/InnocenceEngine/Bin/*.Log | tail -10
   
   # Check for rendering activity
   grep -i "commandlist\|execute\|indirect\|draw" /mnt/c/GitRepo/InnocenceEngine/Bin/[latest-log] | tail -20
   ```

2. **Performance Indicators**:
   - Initialization time <10 seconds = Normal
   - "Default Rendering Client Initialization Task" <6 seconds = Acceptable  
   - No "Task took XXXXXXus" warnings after startup = Good

3. **Success Criteria**:
   ✅ Engine initializes completely ("Engine has been initialized")  
   ✅ DX12 command lists created successfully  
   ✅ No D3D12 validation errors in logs  
   ✅ DrawCall components created for scene objects  
   ✅ No application crashes (process stays running)  

4. **Failure Indicators**:
   ❌ D3D12 errors: "COMMAND_LIST_CLOSED", "ExecuteIndirect failed"  
   ❌ Initialization hangs or takes >30 seconds  
   ❌ Missing "Engine has been initialized" message  
   ❌ GPU validation errors in logs

### Logging Standard
- Use `Log(LogLevel, message)` macro only
- Never use `std::cout`, `printf`, or direct LogService calls

## Common Issues Fixed

1. **Missing activation checks** in Update()
2. **Resource binding index mismatches** between Setup/PrepareCommandList  
3. **Inconsistent logging** approaches
4. **Missing early validation** in Update()
5. **ExecuteIndirect architectural confusion** - Must be in PrepareCommandList (command recording), not ExecuteCommands (command submission)

## Compute Pass Template
```cpp
// Setup: Descriptor index N
m_ResourceBindingLayoutDescs[N].m_DescriptorIndex = N;

// PrepareCommandList: Bind at same index N  
l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, resource, N);
```