const isWinPlatfrom = process.platform === "win32";

let CanApi = function () { };

if (isWinPlatfrom) {
  CanApi = require("./build/Release/can_api.node").CanApi;
}

const createBuffer = require("./helpers/create_buffer");

const CanApiWrapper = function (channel) {
  const CanApiInstance = new CanApi(channel);

  const ifWinPlatformExecute = (propertyName, defaultValue, ...args) => {
    if (isWinPlatfrom) {
      return CanApiInstance[propertyName](...args);
    }

    return defaultValue;
  };

  return {
    readMessage: (...args) => {
      return ifWinPlatformExecute("readMessage", undefined, ...args);
    },
    sendMessage: (...args) => {
      return ifWinPlatformExecute("sendMessage", true, ...args);
    },
    open: (...args) => {
      return ifWinPlatformExecute("open", true, ...args);
    },
    close: (...args) => {
      return ifWinPlatformExecute("close", false, ...args);
    }
  };
};

CanApiWrapper.createBuffer = createBuffer;

module.exports = CanApiWrapper;
