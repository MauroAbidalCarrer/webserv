/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: oboutarf <oboutarf@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/06 14:18:03 by oboutarf          #+#    #+#             */
/*   Updated: 2023/04/06 14:18:46 by oboutarf         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>

int main(int argc, char **argv, char **env)
{
    int i = 0;
    
    printf("Content-Type: text/html\n\n");
    printf("<HTML><HEAD><TITLE>Variables</TITLE></HEAD><BODY>\n");
    
    while(*env){
        printf("%s <BR>\n",*env++);
        
    }
    
    printf("</BODY></HTML>\n");
}