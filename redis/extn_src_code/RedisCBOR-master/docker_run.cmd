docker rm --force redis-cbor
docker run --name redis-cbor -d -p 8001:6379 redis-cbor 
