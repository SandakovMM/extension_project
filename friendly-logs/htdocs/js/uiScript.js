function example(clickedElem)
{
	var res = clickedElem.id;
	var check = clickedElem.checked;
	if (check == false) {
		alert(res + " nononon");
	}
	else {
		alert(res + " yep");
	}
}

function example1(clickedElem)
{
//	var proto = Prototype.Version();
	alert("step zero");
	var id = clickedElem.id;
	alert("step one");
	labels = document.getElementsByTagName("label");
	alert("step two");
	for (var i = 0; i < labels.length; i++) {
		if(labels[i].htmlFor == id) {
			alert(labels[i].value);
			return;
		}
	}/**/
//	alert(cols);
//	var check = clickedElem.checked;
}
