E_APP_PATH=""
if [ "$1" == "node" ]; then ELECTRON_RUN_AS_NODE=1; E_APP_PATH="./test/resources/app.asar"; fi
npm run build
node pack
./test/electron "$E_APP_PATH"
