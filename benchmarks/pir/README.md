# Group-DC PIR Benchmark

本文档说明本仓库中基于 DC 协议实现的两服务器 publicly verifiable PIR 实验。

## 目标

论文中的 PIR 协议把一次私有读取转换成 PVHSS 对选择函数的求值：

```text
f(bin(i)) = sum_j DB[j] * product_l selector(j_l, i_l)
```

其中 `bin(i)` 是客户端查询索引的二进制表示。服务器只能看到这些 bit 的输入编码，不能直接看到 `i`；服务器对数据库执行同态求值后返回带证明的结果份额；客户端公开验证两个份额，再解码得到 `DB[i]`。

本实现先覆盖论文通信分析中的 group-DC 两服务器实例。RLWE-DC PIR 可以复用同样的实验结构，但需要额外实现 RLWE memory value 上的 PIR 选择函数求值。

## 代码结构

新增和修改的主要文件如下：

- `common/include/VHSSElg.h`
  - 增加 `VhssElgamalSubMemory`、`VhssElgamalScaleMemory`。
  - 增加 `VhssElgamalEvaluatePirSelection`。

- `common/src/VHSSElg.cpp`
  - 实现 HSS memory value 的减法和标量乘法。
  - 实现 PIR 选择函数的 prepared selector-tree DP。

- `schemes/dc/group_dc/include/PiDCGroup.h`
  - 增加 `PVHSSElg2_ComputePIR` 声明。

- `schemes/dc/group_dc/src/PiDCGroup.cpp`
  - 增加 `PVHSSElg2_ComputePIR`，把 PIR Eval 的输出接入 DC 的 `Prove`。

- `benchmarks/pir/pir_group_dc.cpp`
  - 独立 PIR benchmark runner。
  - 生成数据库和查询索引。
  - 执行 `Setup/Gen/Query/Answer0/Answer1/Verify/Decode`。
  - 检查 `Decode == DB[index]`。
  - 可选执行篡改实验，确认错误 answer 会被拒绝。
  - 输出在线通信估算。

- `run_pir.sh`
  - 批量扫描不同数据库规模。
  - 写出 `benchmarks/results/pir/pir_group_dc_timing.csv`。

## 选择函数实现

直接按论文公式展开每条记录需要大约 `n log n` 次 HSS 乘法。为了更适合实验，本实现只保留实测最快且正确的 prepared selector-tree DP：

1. 初始前缀是常数 `1` 的 HSS memory value。
2. 对每个查询 bit `x_l`，把每个已有前缀 `p` 拆成：
   - 左分支：`p * (1 - x_l) = p - p*x_l`
   - 右分支：`p * x_l`
3. 遍历 `log n` 层后得到 `n` 个选择项。
4. 计算 `sum_j DB[j] * selector_j`。

这样 HSS 乘法数量约为 `n - 1`，不是 `n log n`。这不改变 PIR 函数语义，只是避免重复计算相同前缀。

索引 bit 使用大端顺序。例如 `n=8`、`index=5` 时，查询 bit 是 `101`，前缀树叶子顺序对应数据库下标 `0..7`。

为了减少每个节点重复处理同一个查询 bit 密文，本实现会为每个查询 bit 预处理一次：

```text
InvMod(Ix[0][0], N^2)
InvMod(Ix[1][0], N^2)
```

原始 HSS multiplication 对形如 `Ix[0][0]^(-e)` 和 `Ix[1][0]^(-e)` 的项会在每次乘法中处理负指数。预处理后，Eval 对已经求好的逆元做正指数幂运算，避免同一层节点反复求逆或处理负指数。这个优化不改变电路语义，也不改变 PRF 调用顺序；每次 HSS 乘法仍然消耗 4 个 PRF 值。

之前测试过 bottom-up MUX 形式，但在当前 DecPed 小非负解码约束下，为了避免负差值必须使用两乘法安全形式 `(1 - x) * left + x * right`，实测慢于 prepared selector-tree。因此代码中只保留 prepared selector-tree。

## 运行

先构建：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

直接运行单个实验：

```bash
build/benchmarks/pir/pir_group_dc \
  --db-size 16 \
  --record-bits 16 \
  --cyctimes 1 \
  --seed 20260615 \
  --tamper \
  --verbose
```

批量运行：

```bash
DB_SIZES=16,32,64,128 CYCTIMES=3 RECORD_BITS=16 ./run_pir.sh
```

输出文件：

```text
benchmarks/results/pir/pir_group_dc_timing.csv
```

常用参数：

- `DB_SIZES`：逗号分隔的数据库大小，必须是 2 的幂，默认 `4,8,16`。
- `CYCTIMES`：每个阶段的计时样本数，默认 `1`。
- `RECORD_BITS`：数据库记录随机 bit 长度，默认 `16`。
- `SEED`：确定性随机种子，默认 `20260615`。
- `TAMPER`：是否执行恶意服务器篡改检测，默认 `1`。

## 输出字段

CSV 主要字段：

- `Setup_ms`：PVHSS/DC 公共参数生成。
- `SetupVhss_ms`：底层 HSS/ElGamal setup。
- `SetupExtra_ms`：DC 证明所需的额外 setup。
- `Gen_ms`：针对 PIR 选择函数上下文生成证明/eval 相关 key。
- `Query_ms`：客户端把 `log n` 个索引 bit 编码成 HSS 输入。
- `Answer0_ms`、`Answer1_ms`：两个服务器完整回答时间。
- `Answer0Eval_ms`、`Answer1Eval_ms`：服务器执行 PIR 选择函数求值的时间。
- `Answer0Proof_ms`、`Answer1Proof_ms`：服务器生成 DC proof 的时间。
- `Verify_ms`：公开验证两个 answer。
- `Decode_ms`：客户端解码出记录值。
- `Correctness`：是否 `Verify == PASS` 且 `Decode == DB[index]`。
- `TamperRejected`：若开启 `--tamper`，是否成功拒绝被篡改的 answer。

通信字段：

- `QueryBitsPerServer`：当前实现中每个服务器收到的在线 query bit 数，按 `log n` 个输入编码、每个编码 4 个 `Z_{N^2}` 元素估算。
- `Answer0Bits`、`Answer1Bits`：按 RELIC 压缩 G1 点长度统计的 answer 大小。每个 answer 是 4 个 G1 点：`C[0], C[1], D[0], D[1]`。
- `TotalOnlineBits`：两个服务器总在线通信量，等于两个 query 加两个 answer。
- `FormulaQueryBitsPerServer`、`FormulaAnswerBitsPerServer`、`FormulaTotalBits`：按论文通信分析口径估算，即 query 约 `8 log n log N_HSS` bits / server，answer 约 `4 log q` bits / server。

这些通信字段只统计在线 query/answer，不把可复用的 setup 参数和 evaluation key 计入每次查询。

## 合理实验设计

建议先做三组实验。

第一组是正确性和恶意检测：

```bash
DB_SIZES=2,4,8,16 CYCTIMES=1 RECORD_BITS=8 TAMPER=1 ./run_pir.sh
```

目标是所有行 `Correctness=PASS` 且 `TamperRejected=PASS`。

第二组是规模扩展：

```bash
DB_SIZES=16,32,64,128,256 CYCTIMES=3 RECORD_BITS=16 ./run_pir.sh
```

重点观察：

- `Query_ms` 随 `log n` 增长。
- `Answer*_Eval_ms` 随 `n` 近似线性增长。
- `Verify_ms` 和 `Decode_ms` 基本不随 `n` 增长。
- `TotalOnlineBits` 随 `log n` 增长。

第三组是记录 bit 长度：

```bash
DB_SIZES=64 CYCTIMES=3 RECORD_BITS=8 ./run_pir.sh
DB_SIZES=64 CYCTIMES=3 RECORD_BITS=16 ./run_pir.sh
DB_SIZES=64 CYCTIMES=3 RECORD_BITS=32 ./run_pir.sh
```

当前 DecPed 解码范围是 `< 2^32`，所以 `RECORD_BITS` 不应超过 32。更大的数据库记录需要拆块、多次查询或额外的编码/打包设计。

## 当前限制

- 目前只实现 group-DC PIR；RLWE-DC PIR 还没有接入选择函数 Eval。
- 数据库大小必须是 2 的幂，这与当前二进制前缀树实现一致。非 2 的幂可以通过 padding 到下一个 2 的幂处理。
- 数据库记录被当作单个小整数，且必须在 DecPed 32-bit 解码范围内。
- 服务器计算时间不是 `O(log n)`。论文中的 `O(log n)` 是在线通信复杂度；本实现的服务器选择函数求值约为 `O(n)` 次 HSS memory 操作。
- 通信统计不包含 setup 阶段参数和 evaluation key 的分发成本，口径与论文中每次查询的在线通信比较一致。
