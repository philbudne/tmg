/* PLB:
 * started with PDP-11 TMG in C tmgl1.t
 *
 * tmgl7a.t compiles/runs under PDP-11 tmg in C, has modifications to
 * output PDP-7 code but uses PDP-11 builtins (ie; parse, not doparse)
 * and therefore character sets must be in -11 format (word per char).
 *
 * tmgl7b.t is initially being created from tmgl7a.t using a series
 * of patch (diff) files to convert the source to be compatible with
 * the available PDP-7 primatives.
 *
 * Ops not used by this file and not present in PDP-7 support code
 * are being reduced/removed.
 *
 * Some rough spots are being smoothed over temporarily
 * by running output through fix7.py
 *
 * NOTE!! This file likely covered by Caldera Ancient Unix licence, see:
 * https://www.bell-labs.com/usr/dmr/www/calderalicense2000.html
 * https://slashdot.org/story/02/01/24/0146248/caldera-releases-original-unices-under-bsd-license
 * https://www.tuhs.org/Archive/Caldera-license.pdf (dated 2002, covers v6)
 * https://www.tech-insider.org/internet/research/2003/0823.html
 */
/****************************************************************/
/* Implementation of TMG in TMGL. */
/* Converts TMGL program into a TMG driving table in C in two phases. */
/* Phase 1: based on the original implementation by M. Douglas McIlroy. */
/* It simplifies further translation by: */
/* - putting the string and character literals on separate lines */
/* - using underscores instead of dots in names and labels */
/* - removing .even and .globl directives */
/* - removing zero-byte from the end of strings */
/* - adding exit bit instead of juxtaposing */
/* - finishing the output with a newline */
/* (c) 2020, Andrii Makukha, 2-clause BSD License. */

begin:	ignore(blanks)
pr1:	comment\pr1
	parse(first)\pr2
	diag(error)
pr2:	comment\pr2
	parse(line)\pr2
	diag(error)\pr2
	parse(last);

first:	parse(( fref = { <st=0100> * <fi=0200> * <start: rc x > 1 *}))
	getfref line = { 2 <:> 1 };

error:	smark ignore(none) any(!<<>>) string(!<<;>>) scopy
	( <;> = {<;>} | null )
	= { * <??? error: > 2 1 * };

line:
	labels
	( statement
	| numbers
	| trule <;> )
	= { 2 * 1 * };

numbers: number <;> numbers/done = { 2 * 1 };

labels:	label labels/done = { 2 * 1 };

label:	name <:> = { 1 <:> };

/* PDP-7 boilerplate: */
last:	= { };

comment: </*>
co1:	ignore(!<<*>>) <*> ignore(none) </>/co1;

statement: [csym=0]
	( prc <)> = (1){2 1 }
	| = (1){} noelem )
stt1:	bundle	( frag = (1){ 2(nil) * 1(q1) }\stt1
		| <;>	( ifelem = { 1(xbit) }
			| ={ 1(nil) * xbit <no;> }
		)	);

prc:	smark ignore(none) <proc(>;

frag:	prule = (1){ 1(nil,q1) }
	| labels noelem = (1){ 1 };

/*in sequel q2 is where to go on fail,q1 is exit bit*/

prule:	[sndt=ndt] disj
	( <|> [ndt=sndt] fref
		( ifeasy prule = (2){3(nil,nil)*<rb >2*
				 1(q2,q1)*2<:>}
		| prule fref = (2){4({*<ra >1},q1)*<goto;>3*
				1<:>2(q2,q1)*3<:>} )
		noelem
	| () );

disj:	pelem pdot
	( disj = (2){2(q2,nil) * 1(nil,q1)} ifelem/done ishard
	| () );

pelem:	pprim = (2){1(q1)$2} iseasy
	| <(>	push(1,sndt)
		( prule <)>
		| <)> = (2){} noelem );

pprim:	( special
	| rname	( <:> fail
		| (spdot|()) ignore(none)
			( <(> ignore(blanks) list(parg) <)>
				= (1){$1 2 * 1}
			| = (1){$1 1}  )))
	( (</> = {<ra >} | <\> = {<rb >})
		rname = (1){3(nil)*$1 2 1}
	| () );

pdot:	<.>/done ignore(none) ident\alias
	fail;
spdot:	<.> ignore(none) not(( any(letter) ))
alias:	fail;

parg:	rname | remote(specparg);

specparg: number
	| <<> lit
	| <*> = { <\n> }
	| <(> ( <)> = { xbit <no;> }
		| push(3,dtt,ndt,sndt) [dtt=0]
			prule <)>
			( ifelem = {1(nil,xbit) }
			| = {1(nil,nil)* xbit <no;>}
		)	);

iseasy:	[easy = 1];
ishard:	[easy = 0];
noelem:	[easy = 2];
ifelem:	[easy!=2?];
ifeasy:	[easy==1?];

special: <=> (rname | remote(trule))
		= (1){ $1 <rt > 1 }
	| <<> literal = (1){ $1 <rx > 1 }
	| <*> = (1){ $1 <rx nl> }
	| <[> expr
		( <?> = {<ro fi>}
		| = {<_p>} )
		<]> = (1){ 2 * $1 1 };

rname:	( name tabval(pat,npa)/done
	| <$> number )
	= { <rs > 1 }; /* PLB: guessing! */

trule:	( tbody
	| <(> (number|tra) <)> tbody = {<gpar;> 2 * 1 } );
tra:	list(tident) octal(npt);

tident:	ident newtab(ptt,npt);

tbody: <{>	( <}> = { xbit <no> }
		| trb);
trb:	telem	( <}> = {  xbit 1 }
		| trb = { 2 * 1 } );

telem:	<<> literal = { <gx > 1 }
	| <*> = { <gx nl> }
	| <$> number = { <gq > 1 }
	| number tdot = tpt
	| name te1\done te2\done;

te1:	tabval(dtt,ndt) tdot = tpt;
te2:	tabval(ptt,npt) = {<gq >1};

tdot:	(<.> number | ={<0>})
	( <(> list(targ) <)> | null)
	= { 2 <;> 1 };

targ:	name|remote(tbody);

/* PDP-7 doesn't take second arg (always zero?) */
tpt:	{ <gp > 2 < " XXX, > 1 };

literal: remote(lit) = { 1 };

lit:	ignore(none) (<>> = { <\> <>> } | null) litb <>>
	 = { <<> 2 1 <>> };

litb:	smark string(litch) scopy <\>/done
	litb = { 2 <\\> 1 };

expr:	assignment | rv ;

assignment: lv assign expr = { 3 * 1 * 2 };

rv:	prime
rv1:	bundle	( infix prime = { 3 * 1 * 2 }\rv1
		| () );

prime:
	lv suffix/done = { 2 * 1 }
	| <&> lv = { <rm > 1 <;ro fi;addr> }
	| <(> expr <)> 
	| unary prime = { 1 * 2 }
	| number = { <rv > 1 };

lv:	( rname = { <_l;> 1 }
	| <(> lv <)>
	| <*> prime = { 1 * <indir> } )
lv1:	<[>/done bundle expr <]> = { 2 * 1 * <_f> }\lv1;

assign:	<=> ignore(none) ( infix = { 1 * <_u> }
			| = { <_st> } );

infix:	smark ignore(none)
	( <+> = {<ad>}
	| <-> = {<sb>}
	| <|> = {<or>}
	| <^> = {<xo>}
	| <&> = {<an>}
	| <==> = {<eq>}
	| <!=> = {<ne>}
	| <<=> = {<le>}
	| <>=> = {<ge>}
	| <<<> = {<sl>}
	| <<> = {<lt>}
	| <>>	(  <>> = {<sr>}
		| = {<gt>} )
	);

suffix:	smark ignore(none)
	( <--> = {<_daXXX>}
	);

unary:	( <-> = {<mi>}
	| <~> = {<cm>}
	);

done:	;

create:	[csym =+ 1]
getcsym: octal(csym) = { <.> 1 };

fref:	[fsym =+ 1]
getfref: octal(fsym) = { <..> 1 };

not:	params(1) $1/done fail;

list:	params(1) $1
list1:	bundle <,>/done $1 = { 2 * 1 }\list1;

remote:	params(1) create parse(rem1,$1);
rem1:	params(1) getcsym $1 = { 2 <=.> * 1 * };

number: smark ignore(none) any(digit) string(digit) scopy;

name:	ident scopy;

ident:	smark ignore(none) any(letter) string(alpha);

newtab:	params(2) fail;

tabval:	params(2) fail;

null:	= nil;

xbit:	{<x >};

q1:	{ $1 };
q2:	{ $2 };

nil:	{};

blanks:	<< 	
	>>;
digit:	<<0123456789>>;
letter:	<<abcdefghijklmnopqrstuvwxyz>>
	<<ABCDEFGHIJKLMNOPQRSTUVWXYZ>>;
alpha:	<<0123456789>>
	<<abcdefghijklmnopqrstuvwxyz>>
	<<ABCDEFGHIJKLMNOPQRSTUVWXYZ>>;
litch:	!<<\>>>;
none:	<<>>;

csym:	0;
fsym:	0;
easy:	0;
w:	0;
n:	0;
dtt:	0;	/*delivered translation table*/
ndt:	0;	/*numb of delivered translations*/
sndt:	0;	/*saved ndt at beginning of disjunctive term*/
pat:	0;	/*parsing rule parameter table*/
npa:	0;	/*number of parsing rule params*/
ptt:	0;	/*table of params of translation*/
npt:	0;	/*number of params of translation*/
index:	0;
