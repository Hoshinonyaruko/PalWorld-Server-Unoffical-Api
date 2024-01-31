# PalWorld-Server-Unofficial-Api

PalWorld-Server-Unofficial-Api is a third-party PalWorld API with the goal of becoming a server-side development API similar to Minecraft Bukkit. The aim is to ensure that plugins do not require updates when the game version is upgraded.

***The project is currently in very early development, and its code may undergo significant rewrites. Therefore, the source code will not be made public for now.***

Currently, I am eager to hear everyone's opinions to decide on what features need to be added. Please share your ideas in the Issues section!

# PalWorld-Server-Unoffical-Api

PalWorld-Server-Unoffical-Api 是一个第三方的帕鲁世界 API， 其目标是成为 Minecraft Bukkit 这样的仅服务端 开发 API，并且希望做到在游戏版本升级时插件无需更新。

***目前项目还在非常早期的开发中， 其代码可能会大规模重写， 所以暂时不会公开源代码***

目前我非常希望听到大家的意见来决定有什么功能需要添加，请在 Issuse 中提出你的想法！

# 改动

尝试加了一个http上去

# 编译

本仓库仅是储存个人改动

首先克隆原始项目 verofess的 PalWorld-Server-Unoffical-Api

首先，去掉wasmtime和funchook的 在 xmake里

然后，补fmt 和spdlog 在3rd

然后chcp 65001

然后修改 xmake lua 

   -- 添加 /bigobj 和忽略特定警告的编译选项

        add_cxflags("/bigobj", "/wd4369", {force = true})

        add_cxxflags("/bigobj", "/wd4369", {force = true})

然后xmake