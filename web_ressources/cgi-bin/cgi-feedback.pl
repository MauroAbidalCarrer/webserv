#!/usr/bin/perl -w


	my $query = $ENV{ "QUERY_STRING" };
	my @pairs = split("&", $query);
	# for (@pairs)	{
	# 	print STDOUT "$_";
	# 	print STDOUT "\n";
	# }
	print STDOUT "HTTP/1.1 200 OK \r\n";
	print STDOUT "Content-type: text/html\r\n";
	print STDOUT "Connection: Keep-Alive\r\n";
	print STDOUT "\r\n";
	print STDOUT "</pre>\n";
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
	print STDOUT "                Votre nom est: \r\n";
	print STDOUT "                </p>\r\n";
	print STDOUT "                <p>\r\n";
	print STDOUT "                  <img src=\"https://media1.giphy.com/media/yJFeycRK2DB4c/200w.gif?cid=6c09b9521topw98b14svvcch9l4sgej4xpy0xw6qa399t41e&rid=200w.gif&ct=g\r\n\">";
	print STDOUT "                </p>\r\n";
	print STDOUT "  </body>\r\n";
	print STDOUT "</html>\r\n";

# ==== PrintRequestedValues =======