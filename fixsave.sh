#!/bin/sh

usage="./fixsave.sh [-o output] [input image]"
outfile=""
infile=""
force=""

while [ "$#" -gt 0 ]
do
    OPTERR=0
    OPTIND=1
    while getopts ":o:f" opt
    do
        case "$opt" in
            o)
                if [ -z "$outfile" ]
                then
                    outfile="$OPTARG"
                else
                    echo "fixsave.sh: -o: parameter passed twice" >&2
                    echo 1
                fi
                ;;
            f)
                force="1"
                ;;
            \?)
                echo "fixsave.sh: invalid option: -$OPTARG" >&2
                exit 1
                ;;
            :)
                echo "fixsave.sh: -$OPTARG: missing argument" >&2
                exit 1
                ;;
        esac
    done
    shift $((OPTIND-1))

    case "$1" in
        -*)
            : ;;
        "")
            : ;;
        *)
            if [ -z "$infile" ]
            then
                infile="$1"
                shift
            else
                echo "fixsave.sh: Too many positional parameters!" >&2
                exit 1
            fi
            ;;
    esac
done

if [ -z "$infile" ]
then
    infile="xdp-image.xpm"
fi

if [ -z "$outfile" ]
then
    if [ $force ]
    then
        outfile="$infile"
    else
        outfile="`echo "$infile" | sed 's/^\(.*\.\)xpm$/\1png/'`"
        if [ x"$outfile" = x"$infile" ]
        then
            echo "fixsave.sh: Specify a different output file with -o or over"\
"write with -f" >&2
            exit 1
        fi
    fi
fi

convert "$infile" -monochrome -colors 2 -depth 1 -negate "$outfile"
