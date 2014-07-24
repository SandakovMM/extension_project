// Call init socket function ol load of document
document.observe('dom:loaded', function(){ SocketWorker(); });

// Class reaction work with on click user actions.
function Reaction() {  }
	//function clickFileCheck(clickedElem)
Reaction.clickFileCheck = function (clickedElem)
{
	// Now geting domain and file like this
	var fileName = clickedElem.value;
	var savedVars = $('someInformation').value;
	var domainName = savedVars.split(' ')[0];
	if (!clickedElem.checked) {
		//SocketWorker.stopReadSend(separated[0], fileName)) // Sending file names to server like that.
		$('setupAll').checked = false;
	}
	else {
		//alert(separated[0] + ' on adress ' + separated[1] + ' and ' + clickedElem.value);
		if (!workSocket) {
			alert('Socket is not initialyze yet');
			//SocketWorker("ws://" + separated[1] + ":10030/"); // Try init socket. // change later
		}
		else {
			SocketWorker.startReadSend(domainName, fileName) // Sending file names to server like that.
		}
	}
}

Reaction.clickAllFiles = function(clickedElem)
{
	// found our label 
	//var elemId = clickedElem.id;
	//var allLabels = $$('label[for=' + elemId + ']')[0]; // sirios sheet need to remember it
	if (!clickedElem.checked) // if we uncheck box
		return;

	var info = $('someInformation').value;
	var domainName = info.split(' ')[0];
	var elemValue = clickedElem.value;
	for (var i = 1; i <= elemValue; i++) {
		var logName = $('log' + i);
		if(!logName.checked) {
			logName.checked = true;
			SocketWorker.startReadSend(domainName, logName.value); // Sending file names to server like that.
		}
	}/**/
}

// Class entry worker add entryes to list and add some css styles to entryes
function EntryWorker() {  }

EntryWorker.addEntry = function(entry)
{
	var list = $('logList');
	var allEntryes = entry.split('\n');
	for (var i = 0; i < allEntryes.length - 1; i++) {
		var newOption = document.createElement('option');
		newOption.innerHTML = allEntryes[i];
		/*newOption.value = /\[.*?\]/.exec(allEntryes[i])[0];
		newOption.value = newOption.value.replace(/[\[\]]/g, "");
		var tryDate = Date.parse(newOption.value);
		alert(tryDate);*/

		newOption.className = EntryWorker.entryAnalyses(allEntryes[i]);
		list.add(newOption, list[0]);
	}
}

EntryWorker.entryAnalyses = function(entry)
{
	/*var someParts = entry.split('[');
	alert(someParts[1] + ' and ' + someParts[2]);*/
	if (/\[ssl:warn\]/.test(entry)) {
		return 'warning'
	}
	if (/\"GET/.test(entry)) {
		return 'good'
	}
	return 'error';
}

//EntryWorker.time

// Class sender work with web socket connections
var workSocket;
function SocketWorker() // Initialyze socket
{ 
	if ("WebSocket" in window) {
		//alert("WebSocket is supported by your Browser!");
		var savedVars = $('someInformation').value;
		var ipAdress = savedVars.split(' ')[1];
     	// Start connection
	    workSocket = new WebSocket("wss://" + ipAdress + ":10020/");
	    workSocket.onopen = function()
	    {
	        // Web Socket is connected, send data using send()
	        alert("Socket is opened");
	        //alert("Start send this: " + host + filename);
	        //workSocket.send('/var/www/vhosts/' + host + '/logs/' + filename);
	        //alert("Message is sent...");
	    };
	    workSocket.onmessage = function (evt) 
	    { 
	    	
	        var received_msg = evt.data;
	        //alert("Message is received: " + received_msg);
	        EntryWorker.addEntry(received_msg);        
	    };
	    workSocket.onclose = function()
	    { 
	        // websocket is closed.
	        alert("Connection is closed..."); 
	    };
	}
	else {
		alert("Web socket is not supported!");
	}
}
SocketWorker.startReadSend = function(host, filename) // function send server file name and start reading logs
{
	//workSocket.send('start'); //Send somthing meens that we need to start reading;
	workSocket.send('add /var/www/vhosts/system/' + host + '/logs/' + filename); // some like this
}
SocketWorker.stopReadSend = function(host, filename) // function send server file name and start reading logs
{
	//workSocket.send('stop'); //Send somthing meens that we need to start reading;
	workSocket.send('del /var/www/vhosts/system/' + host + '/logs/' + filename); // some like this
}
