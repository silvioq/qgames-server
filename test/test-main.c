/*
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * Silvio Quadri 2010.
 * */

#include  <stdlib.h>
#include  <stdio.h>
#include  <assert.h>
#include  <qgames.h>
#include  <qgames_analyzer.h>

int main(int argc, char** argv){

    Tipojuego* tj = qg_tipojuego_new( "Hola" );
    assert( tj );

    exit( EXIT_SUCCESS );
}

