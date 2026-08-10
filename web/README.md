# nano-mimiclaw from Scratch 教学网站

阅读推进时，右侧编辑器逐步补全代码；TraceLab 用真实运行生成的 trace 打断点。

```bash
npm install
npm run dev
```

构建与测试：

```bash
npm run build
npm test
```

内容生成：`npm run generate:content` 从 `../docs` 和 `../src` 生成
`app/content.generated.ts`；`make trace`（在仓库根目录）重新生成 trace 数据。
