include docs/lab.mk

submit:
	@rm -rf *.zip
	@rm -rf ../.submit
	@mkdir -p ../.submit
	@cp -r * ../.submit
	@find . -wholename "*docs/lab*/*.pdf" -exec cp {} ../.submit \;
	@rm -rf submit.zip ../submit.zip
	@cd ../.submit && zip ../submit.zip -r .
	@rm -rf ../.submit
	@mv ../submit.zip .
	-@mv submit.zip $(STUID)_$(AUTHOR)_ARCH实验$(LAB).zip

docs:
	$(MAKE) -C docs

docs-%:
	$(MAKE) -C docs $*

.PHONY: submit docs docs-%