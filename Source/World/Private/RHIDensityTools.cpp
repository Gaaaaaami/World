// Fill out your copyright notice in the Description page of Project Settings.


#include "RHIDensityTools.h"
#include "RHI.h"
#include "RHIResourceUtils.h"
#include "RHITransition.h"
#include "RHIAccess.h"
#include "RHIGPUReadback.h"
#include "DynamicRHI.h"
#include "ShaderParameterStruct.h"
#include "RHICommandList.h"

#if 1
// ---------- Shader ----------
class FDensityNoiseVS : public FGlobalShader
{
    DECLARE_SHADER_TYPE(FDensityNoiseVS, Global);
    FDensityNoiseVS() {}
    FDensityNoiseVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FGlobalShader(Initializer) {
    }
};
IMPLEMENT_SHADER_TYPE(, FDensityNoiseVS,
    TEXT("/Project/NoiseDensity.usf"),
    TEXT("NoiseDensityMainVS"), SF_Vertex);
class FDensityNoisePS : public FGlobalShader
{

public:

   DECLARE_SHADER_TYPE(FDensityNoisePS, Global);

   FDensityNoisePS() {}
   FDensityNoisePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
       : FGlobalShader(Initializer) {

       NoisePosition.Bind(Initializer.ParameterMap, TEXT("NoisePosition"));
       check(NoisePosition.IsBound()); // 建议加这个，开发期直接暴露绑定
   }

    LAYOUT_FIELD(FShaderParameter, NoisePosition);

};
IMPLEMENT_SHADER_TYPE(, FDensityNoisePS,
    TEXT("/Project/NoiseDensity.usf"),
    TEXT("NoiseDensityMainFS"), SF_Pixel);

#else

#endif


URHIDensityTools::URHIDensityTools()
{
}
URHIDensityTools::~URHIDensityTools()
{
}
void URHIDensityTools::Init(int32 InSize)
{
    TextureDesc = FRHITextureDesc(ETextureDimension::Texture2D,
    ETextureCreateFlags::RenderTargetable, EPixelFormat::PF_R8, FClearValueBinding::Transparent, FIntPoint(InSize, InSize), 1, 1, 1, 1, 0);
    TextureCreateDesc = FRHITextureCreateDesc(TextureDesc, ERHIAccess::RTV, TEXT("NoiseDensityGPU_Initialize"));
    Size = InSize;
}
void URHIDensityTools::Draw()
{
    // 必须投递到渲染线程
    ENQUEUE_RENDER_COMMAND(URHIDensityToolsGPU)(
        [this](FRHICommandListImmediate& RHICmdList)
        {

            InitlizeRender(RHICmdList);
            RenderDensityNoise(RHICmdList, FVector3f(0.f, 0.f, 0.f));

            {
                TArray<uint8> buffer;
                int32 RowPitch = 0;
                int32 BufferHeight = 0;
                buffer.SetNumUninitialized(this->Size * this->Size);
                GetPixelBuffer(RHICmdList, buffer, RowPitch, BufferHeight);
                SaveToRawRGBA(buffer.GetData(), 
                    RowPitch, 
                    BufferHeight, 
                    this->Size, 
                    this->Size, 
                    FPaths::Combine(FPaths::ProjectDir(), "image.rgba"));
            }
        }
        );
}

// 保存为原始RGBA数据（Qt可以用QImage::loadFromData或直接解析）
void URHIDensityTools::SaveToRawRGBA(void* CpuData, int32 RowPitch, int32 BufferHeight, int32 Width, int32 Height, const FString& FilePath)
{
    const int32 BytesPerPixel = 4; // BGRA
    const int32 SrcRowBytes = RowPitch;               // GPU每行真实字节数（含填充）
    const int32 DstRowBytes = Width * BytesPerPixel;  // 有效像素每行字节数（无填充）

    TArray<uint8> RawData;
    RawData.SetNumUninitialized(Width * Height * BytesPerPixel);
    check(RawData.Num() == DstRowBytes * Height); // 校验大小，防止算错

    uint8* data = (uint8*)CpuData;
    for (int y = 0; y < BufferHeight; y++)
    {
        FMemory::Memcpy(RawData.GetData() + (y * Width), data + (y * RowPitch), Width);
    }

    uint8* RawDataBinrary = RawData.GetData();

    FFileHelper::SaveArrayToFile(RawData, *FilePath);
    UE_LOG(LogTemp, Log, TEXT("Raw RGBA保存成功: %s | RowPitch=%d, 有效行字节=%d, 总大小=%d"),
        *FilePath, RowPitch, DstRowBytes, RawData.Num());
}

void URHIDensityTools::InitlizeRender(FRHICommandListImmediate& RHICmdList)
{

    const int32 &w = this->Size;
    const int32 &h = this->Size;
    // 2. 包装成RHI创建描述，指定初始访问权限
    // 3. 真正创建离屏RT
    if (!TextureRef.IsValid())
    {

        // 第一次创建Vertex Buffer以及初始化 Shader等操做
        const FRHITextureCreateDesc &CreateDesc = this->TextureCreateDesc;
        this->TextureRef = RHICmdList.CreateTexture(CreateDesc);
        const FTextureRHIRef& RenderTarget = this->TextureRef;

        FRHIDrawIndexedIndirectParameters DrawArgs = { 0 };
        DrawArgs.IndexCountPerInstance = 6;    // 画3个索引（1个三角形）
        DrawArgs.InstanceCount = 1;            // 画1个Instance
        DrawArgs.StartIndexLocation = 0;       // 从IndexBuffer第0位开始
        DrawArgs.BaseVertexLocation = 0;       // 顶点偏移0
        DrawArgs.StartInstanceLocation = 0;    // Instance偏移0

        // 2. 创建 Indirect Buffer，把参数塞进去
    // 注意 Flags 必须是 BUF_DrawIndirect
        DrawArgBuffer = UE::RHIResourceUtils::CreateBufferFromArray(
            RHICmdList,
            TEXT("MyIndirectArgs"),
            EBufferUsageFlags::DrawIndirect, // 关键：标记为 Indirect 参数
            ERHIAccess::IndirectArgs,        // 关键：初始状态是 IndirectArgs
            MakeConstArrayView(&DrawArgs, 1) // 告诉RHI我们只有这一组参数
        );
        const uint16 Indices[6] = { 0, 1, 2 , 1, 3, 2 };
        // 2. 创建 Index Buffer，把数据塞进去
        IndexBuffer = UE::RHIResourceUtils::CreateIndexBufferFromArray(
            RHICmdList,
            TEXT("MyIndexBuffer"),
            MakeConstArrayView(Indices)
        );

        // 1. 定义顶点数据（NDC坐标，和Shader对应）
        // 位置 + W分量
        TArray<FVector4f> Vertices;
        Vertices.Add(FVector4f(-1.0f, -1.0f,0.f, 1.f)); // 左下
        Vertices.Add(FVector4f(1.0f, -1.0f, 1.f, 1.f));  // 右下
        Vertices.Add(FVector4f(-1.0f, 1.0f, 0.f, 0.f));  // 左上
        Vertices.Add(FVector4f(1.0f, 1.0f,  1.f, 0.f));   // 右上
        //// 2. 创建 Vertex Buffer
        VertexBuffer = UE::RHIResourceUtils::CreateVertexBufferFromArray(
            RHICmdList,
            TEXT("MyVertexBuffer"),
            MakeConstArrayView(Vertices)
        );

        // 1. 开始 RenderPass（绑定离屏 RT）

        FRHIRenderPassInfo RPInfo(RenderTarget, ERenderTargetActions::Clear_Store);
        RHICmdList.BeginRenderPass(RPInfo, TEXT("IndirectDrawPass"));
        // 2. 设置视口（和 RT 大小一致）
        RHICmdList.SetViewport(0, 0, 0.0f, w, h, 1.0f);

        // 3. 获取 Shader（用我们之前写的 FIndirectTestVS/PS）
        auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
        auto VS = ShaderMap->GetShader<FDensityNoiseVS>();
        auto PS = ShaderMap->GetShader<FDensityNoisePS>();
        GlobalPS = PS;
        GlobalVS = VS;
        

        FVertexDeclarationElementList Elements;
        Elements.Add(FVertexElement(0, 0, VET_Float4, 0, sizeof(FVector4f), false));

        // 创建RHI资源
        FVertexDeclarationRHIRef VertexDecl = RHICreateVertexDeclaration(Elements);

        // 4. 初始化 PSO（Pipeline State Object）
        // FGraphicsPipelineStateInitializer PSOInit;
        RHICmdList.ApplyCachedRenderTargets(PSOInit);           /** < 将RenderTargetDesc拷贝到PSOInit中 */
        PSOInit.BoundShaderState.VertexDeclarationRHI = VertexDecl;
        PSOInit.BoundShaderState.VertexShaderRHI = VS.GetVertexShader();
        PSOInit.BoundShaderState.PixelShaderRHI = PS.GetPixelShader();
        PSOInit.PrimitiveType = PT_TriangleList;

        // 注意：Indirect 测试通常需要 VertexDeclaration，但我们这次没自定义，先用 nullptr（如果报错我们再补）
        // PSOInit.BoundShaderState.VertexDeclarationRHI = nullptr;
        PSOInit.BlendState = TStaticBlendState<CW_RED>::GetRHI();
        PSOInit.RasterizerState = TStaticRasterizerState<
            FM_Solid,
            CM_None,
            ERasterizerDepthClipMode::DepthClip,
            false  // <-- 关键！
        >::GetRHI();

        PSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
        // 5. 应用 PSO.
        SetGraphicsPipelineState(RHICmdList, PSOInit, 0);
        RHICmdList.SetStreamSource(0, VertexBuffer, 0);


        //{
        //    FVector3f Location(1.f, 1.f, 1.f);
        //    // 1. 从 RHICmdList 获取一个已初始化的批量参数对象
        //    FRHIBatchedShaderParameters& BatchedParameters = RHICmdList.GetScratchShaderParameters();

        //    // 2. 往里塞参数
        //    SetShaderValue(BatchedParameters, PS->NoisePosition, Location);

        //    // 3. 一次性提交
        //    RHICmdList.SetBatchedShaderParameters(PS.GetPixelShader(), BatchedParameters);
        //}

        
        // 6. 绑定UV / Vertex Buffer（把顶点数据喂给 GPU）

        //RHICmdList.DrawIndexedPrimitiveIndirect(
        //    IndexBuffer,
        //    DrawArgBuffer,
        //    0 // Offset: 0
        //);

        RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
        RHICmdList.EndRenderPass();
    }   
}
void URHIDensityTools::RenderDensityNoise(FRHICommandListImmediate& RHICmdList, FVector3f InNoisePosition)
{
    FRHIRenderPassInfo RenderPassInfo(this->TextureRef, ERenderTargetActions::Clear_Store);
    RHICmdList.BeginRenderPass(RenderPassInfo, TEXT("DrawDensityNoise"));
    SetGraphicsPipelineState(RHICmdList, this->PSOInit, 0);
    RHICmdList.SetStreamSource(0, VertexBuffer, 0);
    auto PS = static_cast<FDensityNoisePS *>(GlobalPS.GetShader());
    auto VS = static_cast<FDensityNoiseVS *>(GlobalVS.GetShader());

    FRHIBatchedShaderParameters& BatchedParameters = RHICmdList.GetScratchShaderParameters();
    SetShaderValue(BatchedParameters, PS->NoisePosition, InNoisePosition);
    RHICmdList.SetBatchedShaderParameters(GlobalPS.GetPixelShader(), BatchedParameters);


    RHICmdList.DrawIndexedPrimitiveIndirect(
        IndexBuffer,
        DrawArgBuffer,
        0 // Offset: 0
    );
    RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
    RHICmdList.EndRenderPass();

}

void URHIDensityTools::GetPixelBuffer(FRHICommandListImmediate& RHICmdList, TArray<uint8>& buffer, int32& RowPitch, int32& BufferHeight)
{

    const int32 w = this->Size;
    const int32 h = this->Size;

    FRHITransitionInfo TransitionInfo(TextureRef, ERHIAccess::RTV, ERHIAccess::CopySrc);
    RHICmdList.Transition(TransitionInfo);

    // 纹理回读对象（专门处理纹理）
    TUniquePtr<FRHIGPUTextureReadback> TextureReadback;
    // ✅ 创建回读对象（如果还没创建）
    if (!TextureReadback.IsValid()) {
        TextureReadback = MakeUnique<FRHIGPUTextureReadback>(TEXT("MyRTReadback"));
    }
    TextureReadback->EnqueueCopy(RHICmdList, TextureRef, FIntVector(0.f, 0.f, 0.f), 0, FIntVector(w, h, 1));
  /*  int32 RowPitch = 0;
    int32 BufferHeight = 0;*/
    void* CpuData = TextureReadback->Lock(RowPitch, &BufferHeight);
    check(CpuData); // 肯定成功，因为已经强制刷新了

    //SaveToRawRGBA(CpuData, RowPitch, BufferHeight, w, h, FPaths::Combine(FPaths::ProjectDir(), "image.rgba"));
    memcpy(buffer.GetData(), CpuData, w * h);
    TextureReadback->Unlock();
    TransitionInfo = FRHITransitionInfo(TextureRef, ERHIAccess::CopySrc, ERHIAccess::RTV);
    RHICmdList.Transition(TransitionInfo);
}
