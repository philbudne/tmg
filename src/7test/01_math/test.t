
/* direct assignment not yet working */

program:
	parse(ls2)
	parse(pl1)
	parse(pl1)
	parse(st5)
	parse(an1)
	;


pfoo:	{ <foo: > 1 * };
ls2:	[foo=<<2]	octal(foo) = pfoo;
pl1:	[foo=+1]	octal(foo) = pfoo;
an1:	[foo=&1]	octal(foo) = pfoo;
st5:	[foo=5]		octal(foo) = pfoo;

/* bad results? */
sb1:	[foo=- 1]	octal(foo) = pfoo;

/* crash: */
stmi3:	[foo= -3]	octal(foo) = pfoo;
adfoo:	[foo= &foo]	octal(foo) = pfoo;

foo:	1;
