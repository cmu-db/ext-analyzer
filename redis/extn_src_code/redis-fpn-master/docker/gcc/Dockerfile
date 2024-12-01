
FROM  redis:4.0.11
LABEL maintainer="Artem Labazin <xxlabaza@gmail.com>"


ENV CMAKE_VERSION=3.10.2


RUN apt-get update && \
    apt-get install -y --no-install-recommends build-essential


COPY src /src
COPY Makefile /Makefile


WORKDIR /
ENTRYPOINT ["/usr/bin/make", "build"]
