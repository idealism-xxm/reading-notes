FROM ubuntu:latest
# 安装 fortune
RUN apt-get update && apt-get install -y fortune
# 工作目录为 fortune
WORKDIR /fortune
# 将 fortuneloop.sh 添加到工作目录下
ADD fortuneloop.sh fortuneloop.sh
# 运行该脚本
ENTRYPOINT ["/fortune/fortuneloop.sh"]
# 默认参数为 10
CMD ["10"]
