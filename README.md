# microps (fork)

これは [KLab Expert Camp #6](https://klab-hr.snar.jp/jobboard/detail.aspx?id=ceG7Rw98wQU) にて [pandax381](https://github.com/pandax381) さんの [microps](https://github.com/pandax381/microps) をフォークしたものです。

講義スライドは [TCP/IPプロトコルスタック自作開発](https://drive.google.com/drive/folders/1k2vymbC3vUk5CTJbay4LLEdZ9HemIpZe?usp=sharing) です。

## 使い方

### クローン

```shell
git clone git@github.com:sou1118/microps.git
cd microps
```

### TAP デバイスの作成

```shell
sudo ip tuntap add mode tap user $USER name tap0
sudo ip addr add 192.0.2.1/24 dev tap0
sudo ip link set tap0 up
```

### ビルド & 実行

```shell
# 別のターミナルで先に実行
nc -nv -l 10007
```

```shell
make
./test/step28.exe
```
