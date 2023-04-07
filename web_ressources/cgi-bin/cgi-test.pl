#!/usr/bin/perl

print STDOUT "HTTP/1.1 200 OK \r\n";
print STDOUT "Content-Type: text/html;\r\n";
print STDOUT "Connection: Keep-Alive\r\n";
print STDOUT "\r\n";
	foreach $key (sort keys(%ENV)) {
		print STDOUT "$key = $ENV{$key}<p>";
}
	# $query = $ENV{'QUERY_STRING'};
	# print STDOUT $query;
print STDOUT "</pre>\n";
# my $input = join("", <STDIN>);
# print STDOUT $input;
print STDOUT "<!DOCTYPE html>\r\n";
print STDOUT "<html lang=\"en\">\r\n";
print STDOUT "  <head>\r\n";
print STDOUT "        <meta charset=\"UTF-8\">\r\n";
print STDOUT "        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\r\n";
print STDOUT "        <meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\">\r\n";
print STDOUT "        <title>CGI response</title>\r\n";
print STDOUT "  </head>\r\n";
print STDOUT "  <body>\r\n";
print STDOUT "                <h1>CGI response</h1>\r\n";
print STDOUT "                <p>\r\n";
print STDOUT "                  https://media1.giphy.com/media/yJFeycRK2DB4c/200w.gif?cid=6c09b9521topw98b14svvcch9l4sgej4xpy0xw6qa399t41e&rid=200w.gif&ct=g\r\n";
print STDOUT "                </p>\r\n";
print STDOUT "  </body>\r\n";
print STDOUT "</html>\r\n";