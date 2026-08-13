// Fill out your copyright notice in the Description page of Project Settings.


#include "RHITest.h"
#include "RHI.h"
#include "RHIResourceUtils.h"
#include "RHITransition.h"
#include "RHIAccess.h"
#include "RHIGPUReadback.h"
#include "DynamicRHI.h"
// ---------- Shader ----------
class FIndirectTestVS : public FGlobalShader
{
    DECLARE_SHADER_TYPE(FIndirectTestVS, Global);
    FIndirectTestVS() {}
    FIndirectTestVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FGlobalShader(Initializer) {
    }
};
IMPLEMENT_SHADER_TYPE(, FIndirectTestVS,
    TEXT("/Project/TestDrawInstanced.usf"),
    TEXT("TestDrawInstancedMainVS"), SF_Vertex);

class FIndirectTestPS : public FGlobalShader
{
    DECLARE_SHADER_TYPE(FIndirectTestPS, Global);
    FIndirectTestPS() {}
    FIndirectTestPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
        : FGlobalShader(Initializer) {
    }
};
IMPLEMENT_SHADER_TYPE(, FIndirectTestPS,
    TEXT("/Project/TestDrawInstanced.usf"),
    TEXT("TestDrawInstancedMainPS"), SF_Pixel);


URHITest::URHITest()
{

}

URHITest::~URHITest()
{
}
void URHITest::PostInitProperties()
{

	UObject::PostInitProperties();

}
void URHITest::Draw()
{
    // 必须投递到渲染线程
    ENQUEUE_RENDER_COMMAND(URHITest_DrawTriangle)(
        [this](FRHICommandListImmediate& RHICmdList)
        {
            RenderTest(RHICmdList);
        }
        );
}
// 保存为原始RGBA数据（Qt可以用QImage::loadFromData或直接解析）
void URHITest::SaveToRawRGBA(void* CpuData, int32 RowPitch, int32 Width, int32 Height, const FString& FilePath)
{
    const int32 BytesPerPixel = 4; // BGRA
    const int32 SrcRowBytes = RowPitch;               // GPU每行真实字节数（含填充）
    const int32 DstRowBytes = Width * BytesPerPixel;  // 有效像素每行字节数（无填充）

    TArray<uint8> RawData;
    RawData.SetNumUninitialized(Width * Height * BytesPerPixel);
    check(RawData.Num() == DstRowBytes * Height); // 校验大小，防止算错


    FMemory::Memcpy(RawData.GetData(), CpuData, Width * Height * BytesPerPixel);


    FFileHelper::SaveArrayToFile(RawData, *FilePath);
    UE_LOG(LogTemp, Log, TEXT("Raw RGBA保存成功: %s | RowPitch=%d, 有效行字节=%d, 总大小=%d"),
        *FilePath, RowPitch, DstRowBytes, RawData.Num());
}


void URHITest::RenderTest(FRHICommandListImmediate& RHICmdList)
{

    int w = 3;
    int h = 3;

    FRHITextureDesc RenderTargetDesc(
        ETextureDimension::Texture2D,       // 2D纹理
        ETextureCreateFlags::RenderTargetable, // 标记为可渲染
        PF_G16,                        // 像素格式，和官方一致
        FClearValueBinding(FLinearColor::White), // 默认清屏为黑色
        FIntPoint(w, h),                    // 分辨率，官方用4x4，足够小速度快
        1, 1, 1, 1, 0                       // Mip/数组/采样数等参数，直接抄官方
    );
    // 2. 包装成RHI创建描述，指定初始访问权限
    FRHITextureCreateDesc CreateDesc(RenderTargetDesc, ERHIAccess::RTV, TEXT("RHITest_MyRenderTarget"));
    // 3. 真正创建离屏RT
    FTextureRHIRef RenderTarget = RHICmdList.CreateTexture(CreateDesc);


    FRHIDrawIndexedIndirectParameters DrawArgs = { 0 };
    DrawArgs.IndexCountPerInstance = 3;    // 画3个索引（1个三角形）
    DrawArgs.InstanceCount = 1;            // 画1个Instance
    DrawArgs.StartIndexLocation = 0;       // 从IndexBuffer第0位开始
    DrawArgs.BaseVertexLocation = 0;       // 顶点偏移0
    DrawArgs.StartInstanceLocation = 0;    // Instance偏移0

    // 2. 创建 Indirect Buffer，把参数塞进去
// 注意 Flags 必须是 BUF_DrawIndirect
    FBufferRHIRef DrawArgBuffer = UE::RHIResourceUtils::CreateBufferFromArray(
        RHICmdList,
        TEXT("MyIndirectArgs"),
        EBufferUsageFlags::DrawIndirect, // 关键：标记为 Indirect 参数
        ERHIAccess::IndirectArgs,        // 关键：初始状态是 IndirectArgs
        MakeConstArrayView(&DrawArgs, 1) // 告诉RHI我们只有这一组参数
    );
    const uint16 Indices[3] = { 0, 1, 2 };
    // 2. 创建 Index Buffer，把数据塞进去
    FBufferRHIRef IndexBuffer = UE::RHIResourceUtils::CreateIndexBufferFromArray(
        RHICmdList,
        TEXT("MyIndexBuffer"),
        MakeConstArrayView(Indices)
    );

    // 1. 定义顶点数据（NDC坐标，和Shader对应）
    // 位置 + W分量
    TArray<FVector4f> Vertices;
    Vertices.Add(FVector4f(-0.5f, -0.5f, 0.0f, 1.0f)); // 左下
    Vertices.Add(FVector4f(0.5f, -0.5f, 0.0f, 1.0f)); // 右下
    Vertices.Add(FVector4f(0.0f, 0.5f, 0.0f, 1.0f)); // 顶部（正中间）
    //// 2. 创建 Vertex Buffer
    FBufferRHIRef VertexBuffer = UE::RHIResourceUtils::CreateVertexBufferFromArray(
        RHICmdList,
        TEXT("MyVertexBuffer"),
        MakeConstArrayView(Vertices)
    );
    // 1. 开始 RenderPass（绑定离屏 RT）
    
    FRHIRenderPassInfo RPInfo(RenderTarget, ERenderTargetActions::Clear_Store);
    RHICmdList.BeginRenderPass(RPInfo, TEXT("IndirectDrawPass"));
    // 2. 设置视口（和 RT 大小一致）

    RHICmdList.SetViewport(0, 0, 0.0f, w, h, 1.0f);


#if 1
    // 3. 获取 Shader（用我们之前写的 FIndirectTestVS/PS）
    auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
    TShaderRef<FIndirectTestVS> VS = ShaderMap->GetShader<FIndirectTestVS>();
    TShaderRef<FIndirectTestPS> PS = ShaderMap->GetShader<FIndirectTestPS>();


    FVertexDeclarationElementList Elements;
    Elements.Add(FVertexElement(0, 0, VET_Float4, 0, sizeof(FVector4f), false));
    // 创建RHI资源
    FVertexDeclarationRHIRef VertexDecl = RHICreateVertexDeclaration(Elements);

    // 4. 初始化 PSO（Pipeline State Object）
    FGraphicsPipelineStateInitializer PSOInit;
    PSOInit.BoundShaderState.VertexDeclarationRHI = VertexDecl;

    PSOInit.BoundShaderState.VertexShaderRHI = VS.GetVertexShader();
    PSOInit.BoundShaderState.PixelShaderRHI = PS.GetPixelShader();
    // 注意：Indirect 测试通常需要 VertexDeclaration，但我们这次没自定义，先用 nullptr（如果报错我们再补）
    // PSOInit.BoundShaderState.VertexDeclarationRHI = nullptr;
    PSOInit.PrimitiveType = PT_TriangleList;
    PSOInit.BlendState = TStaticBlendState<>::GetRHI();
    PSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
    PSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

    // 5. 应用 PSO
    RHICmdList.ApplyCachedRenderTargets(PSOInit);           /** < 将RenderTargetDesc拷贝到PSOInit中 */
    SetGraphicsPipelineState(RHICmdList, PSOInit, 0);

    // 6. 绑定 Vertex Buffer（把顶点数据喂给 GPU）
    RHICmdList.SetStreamSource(0, VertexBuffer, 0);
    RHICmdList.DrawIndexedPrimitiveIndirect(
        IndexBuffer,
        DrawArgBuffer,
        0 // Offset: 0
    );


#endif

    RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
    RHICmdList.EndRenderPass();

    // 资源屏障：告诉GPU "RT写完啦，现在我要读它了"
    // 从RT状态转成CopySrc(拷贝源)状态
    // 
    // FRHITransitionInfo(class FRHIBuffer* InRHIBuffer, 
    // ERHIAccess InPreviousState, ERHIAccess InNewState, 
    // EResourceTransitionFlags InFlags = EResourceTransitionFlags::None)

    FRHITransitionInfo TransitionInfo(RenderTarget, ERHIAccess::RTV, ERHIAccess::CopySrc);
    RHICmdList.Transition(TransitionInfo);
 
    // 纹理回读对象（专门处理纹理）
    TUniquePtr<FRHIGPUTextureReadback> TextureReadback;
    // ✅ 创建回读对象（如果还没创建）
    if (!TextureReadback.IsValid()) {
        TextureReadback = MakeUnique<FRHIGPUTextureReadback>(TEXT("MyRTReadback"));
    }
    TextureReadback->EnqueueCopy(RHICmdList, RenderTarget, FIntVector(0.f, 0.f, 0.f), 0, FIntVector(w,h,1));
    int32 RowPitch = 0;
    int32 BufferHeight = 0;
    void* CpuData = TextureReadback->Lock(RowPitch, &BufferHeight);
    check(CpuData); // 肯定成功，因为已经强制刷新了



#if 0
    char *RgbData = (char*)CpuData;
    for (int x = 0; x < RowPitch; x += 4)
    {
        FString n = "";
        for (int y = 0; y < BufferHeight; y++)
        {

            int index = y* RowPitch + x;
            unsigned char r = (unsigned char)RgbData[index];
            unsigned char g = (unsigned char)RgbData[index + 1];
            unsigned char b = (unsigned char)RgbData[index + 2];
            unsigned char a = (unsigned char)RgbData[index + 3];

            n = n + FString::Printf(TEXT("%d, %d, %d, %d"), r, g, b, a);

        }
        UE_LOG(LogTemp, Log, TEXT("%s"), *n);
        UE_LOG(LogTemp, Log, TEXT("================================================================="));

    }
#else
 ///   SaveToRawRGBA(CpuData, RowPitch, w, h, "D:/Image.rgba");
#endif
    TextureReadback->Unlock();
}
