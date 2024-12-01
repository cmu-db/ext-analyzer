
FROM redis:4.0.11
LABEL maintainer="Artem Labazin <xxlabaza@gmail.com>"


RUN mkdir -p /var/lib/redis/data && \
    chmod -R 777 /var/lib/redis


COPY redis.conf /etc/redis.conf
COPY fpn_module-*.so /fpn_module.so


EXPOSE 6379


WORKDIR /
ENTRYPOINT ["/usr/local/bin/redis-server", "/etc/redis.conf", "--loglevel", "verbose"]
