define divideRectangle_favorWidth() {
}

virtualTerminals = {}

//list_append(virtualTerminals, newVT("/bin/bash", 20, 20));

while(true) {
	if(SLang_input_pending(100)) {
	}

	for(i = 0; i < 10; i++) {
		vtRun(i);
	}
	sleep(0);
}
