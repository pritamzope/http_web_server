#!/usr/bin/perl

use strict;
use CGI ':standard';

my $name = param('name');
my $age = param('age');
my $gender = param('gender');
my @hobbies = param('hobby');

my $list;

if (@hobbies) {
$list = join ', ', @hobbies;
} else {
$list = 'None';
}

print header,
start_html(-title=>$name),
h1("Welcome $name"),
p('Here are your details:'),
table(Tr(td('Name:'),
td($name)),
Tr(td('Age:'),
td($age)),
Tr(td('Gender:'),
td($gender)),
Tr(td('Hobbies:'),
td($list))),
end_html;

