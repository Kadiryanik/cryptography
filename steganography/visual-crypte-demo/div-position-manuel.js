var second = document.getElementById("second");

second.style.left = window.innerWidth * 0.4;

// Handle keyboard controls
var keysDown = {};

addEventListener("keydown", function (e) {
	keysDown[e.keyCode] = true;
}, false);

addEventListener("keyup", function (e) {
	delete keysDown[e.keyCode];
}, false);

var keyLeft = 37;
var keyRight = 39;

function draw(){
	// crop px
	var second_left = Number(second.style.left.substring(0, second.style.left.length - 2));

	if (keyLeft in keysDown) {
		second.style.left = (second_left - 1) + 'px';
	}
	if (keyRight in keysDown) {
		second.style.left = (second_left + 1) + 'px';
	}
}

var refInterval = setInterval(draw, 15);