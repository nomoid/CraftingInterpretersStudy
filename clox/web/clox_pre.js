var commandline_log = (function () {
    // Replace console log
    var old = console.log;
    var logger = document.getElementById('log');
    var objectSeen = false;
    return function (message) {
        if(logger !== null){
            if (typeof message == 'object') {
                logger.innerHTML += (JSON && JSON.stringify ? JSON.stringify(message) : message) + '<br />';
            } 
            else if (typeof message == 'string') {
                if (message === 'object' && !objectSeen) {
                    objectSeen = true;
                }
                else {
                    logger.innerHTML += message + '<br />';
                }
            }
            else {
                logger.innerHTML += message + '<br />';
            }
        }
        objectSeen = true;
    }
})();
var commandline_clear = function() {
    document.getElementById('log').innerHTML = '';
}
// Function to clear logs
var clearlogs = function(){
    document.getElementById('log').innerHTML = "";
}
var taboverride = function(e){
    if(e.keyCode==9 || e.which==9){
        e.preventDefault();
        var textArea = e.currentTarget;
        var s = textArea.selectionStart;
        var payload = "    ";
        textArea.value = textArea.value.substring(0,textArea.selectionStart) + payload + textArea.value.substring(textArea.selectionEnd);
        textArea.selectionEnd = s+payload.length; 
    }
}
var Module = {
    noInitialRun: true,
    preRun: [function() {

    }],
    postRun: [],
    print: function(text) {
        if (arguments.length > 1) {
            text = Array.prototype.slice.call(arguments).join(' ');
        }
        console.log(text);
        commandline_log(text)
    },
    printErr: function(text) {
        if (arguments.length > 1) {
            text = Array.prototype.slice.call(arguments).join(' ');
        }
        commandline_log("Error: " + text);
    },
};
window.onerror = function() {
    commandline_log('Exception thrown, see JavaScript console');
};

function runFile(id) {
    commandline_clear();
    var element = document.getElementById(id);
    var inputPath = '/input.lox';
    FS.writeFile(inputPath, element.value);
    Module.ccall('runFile', 'void', ['string'], [inputPath]);
}
