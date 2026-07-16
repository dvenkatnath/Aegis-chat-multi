FROM node:20-alpine

WORKDIR /app

COPY server/package.json server/package-lock.json ./
RUN npm ci --omit=dev

COPY server/keyserver.js ./
COPY deploy/railway/start.sh ./start.sh
RUN chmod +x ./start.sh

RUN mkdir -p /app/dist
COPY index.html ice-config.js cyberknot.js cyberknot.wasm /app/dist/
COPY assets/favicon.svg /app/dist/favicon.svg
COPY assets/icons/favicon-16.png /app/dist/favicon-16.png
COPY assets/icons/favicon-32.png /app/dist/favicon-32.png
COPY assets/icons/favicon-32.png /app/dist/favicon.ico
COPY assets/icons/favicon-48.png /app/dist/logo.png
COPY assets/icons/favicon-180.png /app/dist/apple-touch-icon.png
COPY assets/icons/favicon-192.png /app/dist/favicon-192.png

ENV DATA_DIR=/app/data
ENV STATIC_DIR=/app/dist

EXPOSE 8080
CMD ["sh", "./start.sh"]
