#!/bin/bash

# Colores del arcoíris
colors=(
    "\e[31m"  # rojo
    "\e[91m"  # rojo claro
    "\e[33m"  # amarillo
    "\e[93m"  # amarillo claro
    "\e[32m"  # verde
    "\e[92m"  # verde claro
    "\e[34m"  # azul
    "\e[94m"  # azul claro
    "\e[35m"  # magenta
    "\e[95m"  # magenta claro
    "\e[36m"  # cyan
    "\e[96m"  # cyan claro
)

text=$1
color_index=0

for ((i = 0; i < ${#text}; i++)); do    # ${#text} es la longitud de la cadena
    char="${text:$i:1}"
    if [ "$char" == " " ]; then
        echo -n " "
    else
        echo -en "${colors[$color_index]}$char"
        color_index=$(( (color_index + 1) % ${#colors[@]} ))
    fi
done

echo -e "\e[0m"  # Restaurar el color normal después del texto
