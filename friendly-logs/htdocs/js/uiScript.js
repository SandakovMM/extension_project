// Call init socket function ol load of document
document.observe('dom:loaded', function(){ SocketWorker(); });
var logEntries = [], currentPage = 0, entriesPerPage = 400, pagesCount = 0;
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
		SocketWorker.stopReadSend(domainName, fileName); // Sending file names to server like that.
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

Reaction.clearAll = function()
{
	var list = $('logList');
	var i;
	for (i = list.options.length - 1; i >= 0; i--) {
		list.remove(i);
	}
}

// Class entry worker add entryes to list and add some css styles to entryes
function EntryWorker() {  }

EntryWorker.addEntry = function(entry)
{
	/*var list = $('logList');
	// Generate new option
	var newOption = document.createElement('option');
	newOption.innerHTML = entry;
	
	newOption.className = EntryWorker.entryAnalyses(entry);
	if ($('sorting').checked) {
		newOption.value = /\[.*?\]/.exec(entry)[0];
		newOption.value = EntryWorker.dateWorker(newOption.value);
		var pos = EntryWorker.findPlace(newOption.value);
	} else {
		newOption.value = 0;
		var pos = 0;
	}*/
	//list.add(newOption, list[pos]);
	//logRecords[logRecords.length] = entry;
	if ($('sorting').checked) {
		var value = /\[.*?\]/.exec(entry)[0];
		value = EntryWorker.dateWorker(value);
		var pos = EntryWorker.findPlace(value);
	} else {
		var pos = logEntries.length;
	}
	//list.add(newOption);
	logEntries.splice(pos, 0, entry);

	if (pos < (currentPage + 1) * entriesPerPage || 0 == currentPage || $('sorting').checked){
		goToPage(currentPage);
	}

	if (logEntries.length > pagesCount * entriesPerPage){
		var newPage = document.createElement('a');
		newPage.setAttribute('href', 'javascript:void(0);');
		newPage.setAttribute('onclick', 'goToPage(' + pagesCount + ')');
		newPage.innerHTML = '#' + (pagesCount + 1);
		$('pages').appendChild(newPage);
		pagesCount++;
	}

}
function goToPage(page){
	var logs = $('logList');
	logs.innerHTML = '';
	var start = (pagesCount - page) * entriesPerPage - 1;
	if (start >= logEntries.length)
		start = logEntries.length - 1;
	var end = (pagesCount - page - 1) * entriesPerPage;
	for (i = start; i >= end && i >= 0; i--){
		var newOption = document.createElement('font');
		newOption.innerHTML = logEntries[i] + i + '<br><br>';
		newOption.className = EntryWorker.entryAnalyses(logEntries[i]);
		logs.appendChild(newOption);
	}
	currentPage = page;
}

EntryWorker.entryAnalyses = function(entry) // Add most found entryes.
{
	/*var someParts = entry.split('[');
	alert(someParts[1] + ' and ' + someParts[2]);*/
	if (/\[ssl:warn\]/.test(entry)) {
		return 'warning'
	}
	if (/\[warn/.test(entry)) {
		return 'warning'
	}
	if (/\"GET/.test(entry)) {
		return 'good'
	}
	if (/\"HEAD/.test(entry)) {
		return 'good'
	}
	if (/\"POST/.test(entry)) {
		return 'good'
	}
	return 'error';
}

EntryWorker.dateWorker = function(dateEntry) // Function to make normal time from log.
{
	var result = dateEntry.replace(/[\[\]]/g, '');
	if (/^.*?\s[-+].*$/.test(result)) { // if it's log from standart access.
		var timeShift = result.replace(/^.*?\s([-+].*)$/, '$1');
		var result = result.replace(/^(.*?)\s[-+].*$/, '$1'); // Serios SHIT!
		var detachF = result.split('/');
		var detachS = detachF[2].split(':');
		var resDate = Date.parse(detachF[1] + ' ' + detachF[0] + ' ' + detachS[0] + ' ' + 
					detachS[1] + ':' + detachS[2] + ':' + detachS[3] + ' GMT' + timeShift);
	}
	else {// seems like error log. Later need find other shit.
		var detachF = result.split(' ');
		var detachS = detachF[3].split('.');
		var resDate = Date.parse(detachF[0] + ' ' + detachF[1] + ' ' + detachF[2] + ' ' + 
					detachF[4] + ' ' + detachS[0]);
	}
	return resDate;
}

EntryWorker.findPlace = function(dateEntry) // Function to make normal time from log.
{
	var allOptions = $$('select#logList option');
	var i = 0;
	for (; i < allOptions.length; i++) {
		if (allOptions[i].value <= dateEntry)
			return i;
	}
	return i;
}

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
