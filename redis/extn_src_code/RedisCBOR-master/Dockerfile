# Setup
FROM rust as setup
RUN apt-get update
RUN apt-get install -y clang
ENV LIBCLANG_PATH=/usr/lib/x86_64-linux-gnu
WORKDIR /build

# Build
FROM setup as build
COPY src ./src
COPY Cargo.toml .
ENV CARGO_REGISTRIES_CRATES_IO_PROTOCOL=sparse
RUN cargo build --release

# Runtime
FROM redis
WORKDIR /data
COPY --from=build /build/target/release/librecbor.so /
CMD ["redis-server", "--loadmodule", "/librecbor.so"]