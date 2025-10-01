const float pi = 3.14159265358979323846f;

float easeOutBounce(float x) {
    const float n1 = 7.5625;
    const float d1 = 2.75;

    if (x < 1 / d1) {
    	return n1 * x * x;
    }
     else if (x < 2 / d1) {
        return n1 * (x -= 1.5f / d1) * x + 0.75f;
    }
    else if (x < 2.5 / d1) {
        return n1 * (x -= 2.25f / d1) * x + 0.9375f;
    }
    else {
        return n1 * (x -= 2.625f / d1) * x + 0.984375f;
    }
}

float easeInOutCubic(float x) {
	return x < 0.5 ? 4 * x * x * x : 1 - pow(-2 * x + 2, 3) / 2;
}

float easeOutElastic(float x) {
	const float  c4 = (2 * pi) / 3;
	
	return x == 0
	  ? 0
	  : x == 1
	  ? 1
	  : pow(2, -10 * x) * sin((x * 10 - 0.75) * c4 * 1.0f) * 2.0f + 1;
}