FROM docker.io/alpine:3.21
RUN apk update && apk add libmicrohttpd-dev hiredis-dev alpine-sdk && mkdir /workspace
WORKDIR /workspace
COPY src/main.c /workspace/main.c
RUN c99 -o server main.c -lmicrohttpd -lhiredis
CMD /workspace/server 8080
