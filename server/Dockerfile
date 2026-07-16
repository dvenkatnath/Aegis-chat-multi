FROM node:20-alpine
WORKDIR /app
COPY package.json ./
RUN npm install --omit=dev
COPY keyserver.js ./
ENV DATA_DIR=/app/data
EXPOSE 8080
CMD ["node", "keyserver.js"]
