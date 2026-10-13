@echo off
chcp 65001 >nul
title VoxelTerrain - 文件结构生成器

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "PLUGIN_DIR=%ROOT%\Plugins\VoxelTerrain"
set "BASE=%PLUGIN_DIR%\Source\VoxelTerrain"

echo.
echo ============================================================
echo   VoxelTerrain 文件结构生成器
echo ============================================================
echo   项目根目录 : %ROOT%
echo   插件目录   : %PLUGIN_DIR%
echo   模块目录   : %BASE%
echo ============================================================
echo.

REM ---------- 检查插件 ----------
if not exist "%PLUGIN_DIR%\VoxelTerrain.uplugin" goto :no_plugin
goto :create

:no_plugin
echo [错误] 未找到 VoxelTerrain 插件
echo        预期位置 - %PLUGIN_DIR%\VoxelTerrain.uplugin
echo.
echo        请先在 UE 编辑器中创建名为 VoxelTerrain 的空白插件
echo        菜单 - 编辑 - 插件 - 添加 - 空白
echo.
pause
exit /b 1

REM ---------- 创建目录 ----------
:create
echo [1/3] 创建子目录...
if not exist "%BASE%\Public"  mkdir "%BASE%\Public"
if not exist "%BASE%\Private" mkdir "%BASE%\Private"
echo       OK
echo.

REM ---------- Public 头文件 ----------
echo [2/3] 创建 Public 头文件 8 个...
call :MakeHeader "%BASE%\Public\VoxelTypes.h"            "FVoxel - FChunkKey - 全局常量"
call :MakeHeader "%BASE%\Public\VoxelChunkData.h"        "FVoxelChunkData - FMeshVertex - FVoxelMeshData"
call :MakeHeader "%BASE%\Public\VoxelGenerator.h"        "噪声地形生成器"
call :MakeHeader "%BASE%\Public\VoxelSceneProxy.h"       "SceneProxy"
call :MakeHeader "%BASE%\Public\VoxelWorldSubsystem.h"   "流式加载调度器"
call :MakeHeader "%BASE%\Public\VoxelTerrainComponent.h" "地形渲染组件"
call :MakeHeader "%BASE%\Public\TransvoxelMesher.h"      "网格生成器接口"
call :MakeHeader "%BASE%\Public\TransvoxelTables.h"      "Transvoxel 官方查找表"
echo       OK
echo.

REM ---------- Private 实现文件 ----------
echo [3/3] 创建 Private 实现文件 6 个...
call :MakeCpp "%BASE%\Private\VoxelGenerator.cpp"        "噪声地形生成器实现"
call :MakeCpp "%BASE%\Private\VoxelSceneProxy.cpp"       "SceneProxy 实现"
call :MakeCpp "%BASE%\Private\VoxelWorldSubsystem.cpp"   "流式加载调度器实现"
call :MakeCpp "%BASE%\Private\VoxelTerrainComponent.cpp" "地形渲染组件实现"
call :MakeCpp "%BASE%\Private\TransvoxelMesher.cpp"      "网格生成器实现"
call :MakeCpp "%BASE%\Private\VoxelTerrainModule.cpp"    "模块入口"
echo       OK
echo.

echo ============================================================
echo   完成
echo ============================================================
echo.
echo   生成的文件树
echo.
echo   Plugins\VoxelTerrain\Source\VoxelTerrain\
echo     Public\
echo       VoxelTypes.h
echo       VoxelChunkData.h
echo       VoxelGenerator.h
echo       VoxelSceneProxy.h
echo       VoxelWorldSubsystem.h
echo       VoxelTerrainComponent.h
echo       TransvoxelMesher.h
echo       TransvoxelTables.h
echo     Private\
echo       VoxelGenerator.cpp
echo       VoxelSceneProxy.cpp
echo       VoxelWorldSubsystem.cpp
echo       VoxelTerrainComponent.cpp
echo       TransvoxelMesher.cpp
echo       VoxelTerrainModule.cpp
echo.
echo ============================================================
echo   下一步
echo ============================================================
echo.
echo   1. 把 Transvoxel.h 内容粘贴到 Public\TransvoxelTables.h
echo      并且把所有 long 替换成 int32
echo.
echo   2. 从聊天记录里把每个文件的实现粘贴进去
echo.
echo   3. 替换 Private\VoxelTerrainModule.cpp 为两行
echo         #include "Modules/ModuleManager.h"
echo         IMPLEMENT_MODULE(FDefaultModuleImpl, VoxelTerrain);
echo.
echo   4. 右键 .uproject 生成 VS 工程文件
echo.
echo   5. 编译 Development Editor
echo.
pause
exit /b 0

REM ============================================================
REM  子过程 - 创建头文件
REM ============================================================
:MakeHeader
if exist "%~1" (
    echo       [跳过] %~nx1
    exit /b 0
)
>"%~1"  echo #pragma once
>>"%~1" echo // %~2
exit /b 0

REM ============================================================
REM  子过程 - 创建 cpp 文件
REM ============================================================
:MakeCpp
if exist "%~1" (
    echo       [跳过] %~nx1
    exit /b 0
)
>"%~1" echo // %~2
exit /b 0