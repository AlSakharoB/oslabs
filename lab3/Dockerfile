# Dockerfile
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /lab5

COPY . .

# Проверяем при сборке, что всё компилится и тесты проходят
RUN make clean && make server && make client && make test

CMD ["/bin/bash"]
