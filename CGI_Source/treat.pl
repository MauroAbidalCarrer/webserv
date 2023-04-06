#!/usr/bin/perl

# les donnees sont envoyees par methode GET
# donc on recupere les donnees dans la variable 
# d'environnement QUERY_STRING
$buffer=$ENV{"QUERY_STRING"};

# on split la chaine de donnees en des paires name=value
local(@champs) = split(/&/, $buffer);
local($donnees) = "";

# affichage du debut du code HTML
printf STDOUT "Content-type: text/html\n\n";
printf STDOUT "<HTML><HEAD>";
printf STDOUT "<TITLE>Reponse au questionnaire</TITLE>";
printf STDOUT "</HEAD>";
printf STDOUT "<BODY BGCOLOR=\"#ffffff\">";

printf STDOUT "<H1>Résultat du traitement de votre questionnaire</H1>";
printf STDOUT "<H2>Chaine de données reçue par le programme</H2>";
printf STDOUT "QUERY_STRING <STRONG>%s</STRONG>",$buffer;
printf STDOUT "<H2>Liste des informations décodées</H2>";
printf STDOUT "<UL>";
printf STDOUT "<BL>";

# recuperation et mise en forme des donnees
# on parcourt la liste des paires name=value
foreach $i (0 .. $#champs) {
    # On convertit les plus en espaces
    $champs[$i] =~ s/\+/ /g;
    
    # On separe chaque champ en une cle et sa valeur
    ($key, $val) = split(/=/,$champs[$i],2); 
    
    # On convertit les %XX de leur valeur hexadecimale en alphanumerique
    $key =~ s/%(..)/pack("c",hex($1))/ge;
    $val =~ s/%(..)/pack("c",hex($1))/ge;

    # on affiche le resultat
    printf STDOUT "<LI><STRONG>%s:</STRONG>%s\n",$key,$val;
}

printf STDOUT "</BL>";
printf STDOUT "</UL>";

printf STDOUT "</BODY>";
printf STDOUT "</HTML>";
