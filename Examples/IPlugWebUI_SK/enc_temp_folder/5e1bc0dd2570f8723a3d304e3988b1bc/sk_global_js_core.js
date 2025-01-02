console.log('sk_global_js_core.js')

//Fundamental core functions
function getDirname(asArray) {
    var stack = (new Error()).stack
    //stack = console.trace()
    var firstCaller = stack.split('\n').at(-1)
    var split1 = firstCaller.split('')
    var el1 = split1[1]
    var split2 = el1.split(':')
    var el2 = split2[0]
    var split3 = el2.split('/')
    var trimmedPath = split3
    trimmedPath.splice(trimmedPath.length - 1, 1)
    trimmedPath = trimmedPath.join('/')

    return trimmedPath
}

Object.defineProperty(window, '__dirname', {
    get() {
        return getDirname()
    },
});






window.__sk_global_core = {
  //Called at beginning of user web content to ensure that the global core has been initialized
  isInitialized: (timeout = 3000) => {
    return new Promise((resolve, reject) => {
      if (sk_api) return resolve()

      var start_ms = Date.now()

      var await_sk_core_timer = setInterval(() => {

        var end_ms = Date.now()
        var elapsed_ms = end_ms - start_ms
        if (elapsed_ms > timeout) {
          clearInterval(await_sk_core_timer)
          var err = "[SK Global Core] Was never initialized in time"
          console.error(err)
          return reject(err)
        }


        if (!sk_api) return
          if (!sk_api.initialized) return

          clearInterval(await_sk_core_timer)
        console.log("[SK Global Core] Initialized")
        resolve()
      }, 1)
    })
  }
}

class SK_Global_Core {
    constructor(opt){
        this.initialized = false

        this.id = opt.id


        this.staticInfo = {
            machine: '<sk_machine_info>',
            application: '<sk_application_info>'
        }


        this.nativeModules = {
            node: { fs: __dirname + '/node/fs.js' }
        }


        this.init()
    }

    init(){
        this.ipc = new SK_IPC({
          sender_id: this.id
        })

        this.initialized = true

        console.log('SK Global Core successfully initialized. Accessable via sk_api')
    }

    fetch(path, data, onPreParse){
        /*
        
            maybe need to format path to handle certain scenarios such as:
    
            ./targetFile.js
            ./ targetFile       without extension
            ../                 walk up the path - !!! IMPORTANT !!! walking up a path may not exceed the root of the "assets" folder
            \                   non-unix path delimiters
        
        */

        var finalPath = path
        if (data) path += '!' + btoa(JSON.stringify(data))

        const request = new XMLHttpRequest()
        try {
            request.open('GET', path, false)
            request.send()
        } catch (err) {
            console.error(err)
            throw 'Could not fetch module at ' + path
        }

        var response = undefined

        try { response = request.responseText } catch { response = request.response }

        if (request.getAllResponseHeaders().indexOf('application/json') > -1) {
            if (!onPreParse) response = JSON.parse(response)
            else response = onPreParse(response)
        }

        return response
    }

    initModules(){
        var fs = require('fs')
        var categories = fs.readdirSync('/')

        for (var i in categories) {
            var catName = categories[i]

            if (!sk_api.nativeModules[catName]) sk_api.nativeModules[catName] = {}

            var moduleCategory = fs.readdirSync('/' + catName + '/')
            for (var u in moduleCategory) {
                var modName = moduleCategory[u].split('.')[0]
                sk_api.nativeModules[catName][modName] = catName + '/' + modName + '.js'
            }
        }
    }
}

window.sk_api = new SK_Global_Core({ id: Date.now().toString() })