var first = document.getElementById("first");
var second = document.getElementById("second");

//first.style.left = window.innerWidth * 0.3;
second.style.left = window.innerWidth * 0.7;

var v = 1;

function draw(){
	// crop px
	var first_left = Number(first.style.left.substring(0, first.style.left.length - 2));
	var second_left = Number(second.style.left.substring(0, second.style.left.length - 2));

	if(first_left < second_left){
		//first.style.left = (first_left + v) + 'px';
		second.style.left = (second_left - v) + 'px';
	}else{
		second.style.left = first.style.left;
		// stop the interval
		clearInterval(refInterval);
	}
}

var refInterval = setInterval(draw, 1);