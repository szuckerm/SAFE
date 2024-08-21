all: docs

docs: 
	mkdir -p html
	NaturalDocs -i .. -o HTML ./html -p ./naturaldocs -r -hl all -s Style

clean:
	rm -rf naturaldocs/data
