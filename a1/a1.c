#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <fcntl.h>

#define NO_ARGUMENTS 1
#define TOO_FEW_PARAMETERS 2
#define TOO_MANY_PARAMETERS 3
#define USAGE_ERROR 4
#define PATH_TOO_LONG 5
#define NAME_TOO_LONG 6
#define SIZE_TOO_BIG 7
#define INVALID_SIZE 8
#define LOCATION_NOT_ACCESSIBLE 9
#define NOT_A_DIRECTORY 10
#define DOES_NOT_COMPLY 11
#define NaN 12
#define LINE_DOES_NOT_EXIST 13
#define UNKNOWN_COMMAND 20

typedef struct list_parameters
{
	char path[1024];
	char name_start[256];
	unsigned long long size_limit;
	char filter_type;
	char recursive;
	short recursion_level;
}
	list_parameters;

typedef struct extract_parameters
{
	char path[1024];
	int sect_nr;
	int line;
}
	extract_parameters;

typedef struct section_header
{
	char name[14];
	short sect_type;
	long int sect_offset;
	long int sect_size;
}
	section_header;

typedef struct SF_header
{
	char version;
	char no_of_sections;
	section_header *section_headers;
	int header_size;
	char magic[2];
}
	SF_header;

void swap_arguments ( char **a, char **b )
{
	char *aux = *a;
	*a = *b;
	*b = aux;
}

void sort_list_parameters ( int argc, char **argv )
{
	argc -= 2;
	argv += 2;

	if ( argc == 1 )
	{
		if ( strncmp ( argv[0], "path=", 5 ) != 0 )
		{
			USAGE_ERROR_LABEL:
			printf ( "ERROR\nUSAGE: ./a1 list [recursive] <filtering_options> path=<dir_path>\n" );
			exit ( USAGE_ERROR );
		}
	}
	else if ( argc == 2 )
	{
		if ( strncmp ( argv[0], "path=", 5 ) == 0 )
			swap_arguments ( argv, argv + 1 );

		if ( ( strcmp ( argv[0], "recursive" ) && strncmp ( argv[0], "size_greater=", 13 ) &&
			   strncmp ( argv[0], "name_starts_with=", 17 ) ) || strncmp ( argv[1], "path=", 5 ) )
			goto USAGE_ERROR_LABEL;
	}
	else
	{
		if ( strncmp ( argv[0], "path=", 5 ) == 0 )
			swap_arguments ( argv, argv + 2 );
		else if ( strncmp ( argv[1], "path=", 5 ) == 0 )
			swap_arguments ( argv + 1, argv + 2 );

		if ( strcmp ( argv[1], "recursive" ) == 0 )
			swap_arguments ( argv, argv + 1 );

		if ( strcmp ( argv[0], "recursive" ) ||
			 ( strncmp ( argv[1], "size_greater=", 13 ) && strncmp ( argv[1], "name_starts_with=", 17 ) ) ||
			 strncmp ( argv[2], "path=", 5 ) )
			goto USAGE_ERROR_LABEL;
	}
}

unsigned long long file_size ( char *path )
{
	struct stat stat_struct;
	stat ( path, &stat_struct );
	return ( unsigned long long ) stat_struct.st_size;
}

void prepare_list_parameters ( struct list_parameters *list_param, int argc, char **argv )
{

	list_param->filter_type = 0;
	list_param->recursive = 0;
	list_param->recursion_level = 0;

	if ( strncmp ( argv[argc - 1], "path=", 5 ) || strlen ( argv[argc - 1] ) < 5 )
	{
		printf ( "ERROR\nUSAGE: ./a1 list [recursive] <filtering_options> path=<dir_path>\n" );
		exit ( USAGE_ERROR );
	}

	if ( strlen ( argv[argc - 1] ) > 1024 + 5 )
	{
		printf ( "ERROR\nmax path length exceeded\n" );
		exit ( PATH_TOO_LONG );
	}

	strcpy ( list_param->path, argv[argc - 1] + 5 );

	if ( argc == 4 )
	{
		if ( strncmp ( argv[argc - 2], "size_greater=", 13 ) == 0 && strlen ( argv[argc - 2] ) > 13llu )
		{
			if ( strlen ( argv[argc - 2] + 13 ) > 256 )
			{
				SIZE_TOO_BIG_LABEL:
				printf ( "ERROR\nsize limit exceeds unsigned long long int bounds\n" );
				exit ( SIZE_TOO_BIG );
			}

			list_param->size_limit = strtoull ( argv[argc - 2] + 13, NULL, 0 );

			if ( list_param->size_limit == 0 )
			{
				printf ( "ERROR\nsize limit must be a positive integer\n" );
				exit ( INVALID_SIZE );
			}
			else if ( list_param->size_limit == ULLONG_MAX )
				goto SIZE_TOO_BIG_LABEL;

			list_param->filter_type = 's';
		}
		else if ( strncmp ( argv[argc - 2], "name_starts_with=", 17 ) == 0 && strlen ( argv[argc - 2] ) > 17llu )
		{
			if ( strlen ( argv[argc - 2] + 17 ) > 256 )
			{
				printf ( "ERROR\nmax filename length exceeded\n" );
				exit ( NAME_TOO_LONG );
			}
			strcpy ( list_param->name_start, argv[argc - 2] + 17llu );
			list_param->filter_type = 'n';
		}
		else
			list_param->recursive = 'r';
	}
	else if ( argc == 5 )
	{
		if ( strncmp ( argv[argc - 2], "size_greater=", 13 ) == 0 && strlen ( argv[argc - 2] ) > 13llu )
		{
			if ( strlen ( argv[argc - 2] + 13 ) > 256 )
			{
				printf ( "ERROR\nmax filename length exceeded\n" );
				exit ( NAME_TOO_LONG );
			}

			list_param->size_limit = strtoull ( argv[argc - 2] + 13, NULL, 0 );

			if ( list_param->size_limit == 0 )
			{
				printf ( "ERROR\nsize limit must be a positive integer\n" );
				exit ( INVALID_SIZE );
			}
			else if ( list_param->size_limit == ULLONG_MAX )
				goto SIZE_TOO_BIG_LABEL;

			list_param->filter_type = 's';
		}
		else if ( strncmp ( argv[argc - 2], "name_starts_with=", 17 ) == 0 && strlen ( argv[argc - 2] ) > 17llu )
		{
			strcpy ( list_param->name_start, argv[argc - 2] + 17llu );
			list_param->filter_type = 'n';
		}

		list_param->recursive = 'r';
	}
}

void list ( struct list_parameters *list_param )
{
	char aux[1024] = {0};
	struct list_parameters new_list_param;

	if ( access ( list_param->path, F_OK ) )
	{
		printf ( "ERROR\n" );
		perror ( "" );
		exit ( LOCATION_NOT_ACCESSIBLE );
	}

	struct stat stat_struct;
	stat ( list_param->path, &stat_struct );

	if ( S_ISDIR ( stat_struct.st_mode ) == 0 )
	{
		printf ( "ERROR\ngiven path leads to something other than a directory\n" );
		exit ( NOT_A_DIRECTORY );
	}

	DIR *directory = opendir ( list_param->path );
	struct dirent *entry;

	if ( list_param->recursion_level == 0 )
		printf ( "SUCCESS\n" );

	if ( list_param->filter_type == '\0' )
	{
		while ( ( entry = readdir ( directory ) ) != NULL )
			if ( strcmp ( entry->d_name, "." ) && strcmp ( entry->d_name, ".." ) )
			{
				printf ( "%s/%s\n", list_param->path, entry->d_name );

				strcpy ( aux, list_param->path );
				strcat ( aux, "/" );
				strcat ( aux, entry->d_name );

				stat ( aux, &stat_struct );

				if ( list_param->recursive == 'r' && S_ISDIR ( stat_struct.st_mode ) )
				{

					strcpy ( new_list_param.path, list_param->path );
					strcat ( new_list_param.path, "/" );
					strcat ( new_list_param.path, entry->d_name );

					new_list_param.filter_type = list_param->filter_type;

					new_list_param.recursive = 'r';
					new_list_param.recursion_level = list_param->recursion_level + 1;

					list ( &new_list_param );
				}
			}
	}
	else if ( list_param->filter_type == 'n' )
	{
		while ( ( entry = readdir ( directory ) ) != NULL )
			if ( strcmp ( entry->d_name, "." ) && strcmp ( entry->d_name, ".." ) )
			{
				if ( strncmp ( entry->d_name, list_param->name_start, strlen ( list_param->name_start ) ) == 0 )
					printf ( "%s/%s\n", list_param->path, entry->d_name );

				strcpy ( aux, list_param->path );
				strcat ( aux, "/" );
				strcat ( aux, entry->d_name );

				stat ( aux, &stat_struct );

				if ( list_param->recursive == 'r' && S_ISDIR ( stat_struct.st_mode ) )
				{

					strcpy ( new_list_param.path, list_param->path );
					strcat ( new_list_param.path, "/" );
					strcat ( new_list_param.path, entry->d_name );

					strcpy ( new_list_param.name_start, list_param->name_start );
					new_list_param.filter_type = list_param->filter_type;

					new_list_param.recursive = 'r';
					new_list_param.recursion_level = list_param->recursion_level + 1;

					list ( &new_list_param );
				}
			}
	}
	else
		while ( ( entry = readdir ( directory ) ) != NULL )
		{
			strcpy ( aux, list_param->path );
			strcat ( aux, "/" );
			strcat ( aux, entry->d_name );
			if ( strcmp ( entry->d_name, "." ) && strcmp ( entry->d_name, ".." ) )
			{
				if ( file_size ( aux ) > list_param->size_limit )
					printf ( "%s/%s\n", list_param->path, entry->d_name );

				strcpy ( aux, list_param->path );
				strcat ( aux, "/" );
				strcat ( aux, entry->d_name );

				stat ( aux, &stat_struct );

				if ( list_param->recursive == 'r' && S_ISDIR ( stat_struct.st_mode ) )
				{

					strcpy ( new_list_param.path, list_param->path );
					strcat ( new_list_param.path, "/" );
					strcat ( new_list_param.path, entry->d_name );

					new_list_param.filter_type = list_param->filter_type;
					new_list_param.size_limit = list_param->size_limit;

					new_list_param.recursive = 'r';
					new_list_param.recursion_level = list_param->recursion_level + 1;

					list ( &new_list_param );
				}
			}
		}

	closedir ( directory );
}

void prepare_parse_parameters ( char *path, int argc, char **argv )
{
	if ( strncmp ( argv[2], "path=", 5 ) || strlen ( argv[2] ) < 6 )
	{
		printf ( "ERROR\nUSAGE: ./a1 parse path=file_path>\n" );
		exit ( USAGE_ERROR );
	}

	if( strlen( argv[2]) - 5 > 1023)
	{
		printf("ERROR\nmax path length exceeded\n");
		exit(PATH_TOO_LONG);
	}

	strcpy ( path, argv[argc - 1] + 5 );
}

int parse ( char *path, SF_header *header )
{
	if ( access ( path, F_OK ) )
		return 1;

	if ( file_size ( path ) < 69 )
		return 2;

	unsigned char header_size_str[2];

	int fd = open ( path, O_RDONLY );

	lseek ( fd, -4, SEEK_END );

	read ( fd, header_size_str, 2 );
	read ( fd, header->magic, 2 );

	if ( strncmp ( header->magic, "Nu", 2 ) )
		return 3;

	header->header_size = 0;
	header->header_size += ( int ) header_size_str[1] * 256;
	header->header_size += ( int ) header_size_str[0];

	lseek ( fd, -header->header_size, SEEK_END );

	read ( fd, &( header->version ), 1 );
	if ( header->version < 15 || header->version > 51 )
		return 4;

	read ( fd, &( header->no_of_sections ), 1 );
	if ( header->no_of_sections < 3 || header->no_of_sections > 19 )
		return 5;

	int i = 0;
	unsigned char sect_type[2];
	unsigned char sect_offset[4];
	unsigned char sect_size[4];
	header->section_headers = calloc ( header->no_of_sections, sizeof ( section_header ) );

	for ( ; i < ( int ) header->no_of_sections; i++ )
	{
		read ( fd, header->section_headers[i].name, 13 );
		read ( fd, sect_type, 2 );
		read ( fd, sect_offset, 4 );
		read ( fd, sect_size, 4 );

		header->section_headers[i].sect_type += ( int ) sect_type[1] * 256;
		header->section_headers[i].sect_type += ( int ) sect_type[0];

		header->section_headers[i].sect_offset += ( long int ) sect_offset[3] * 16777216l;
		header->section_headers[i].sect_offset += ( long int ) sect_offset[2] * 65536l;
		header->section_headers[i].sect_offset += ( long int ) sect_offset[1] * 256l;
		header->section_headers[i].sect_offset += ( long int ) sect_offset[0];

		header->section_headers[i].sect_size += ( long int ) sect_size[3] * 16777216l;
		header->section_headers[i].sect_size += ( long int ) sect_size[2] * 65536l;
		header->section_headers[i].sect_size += ( long int ) sect_size[1] * 256l;
		header->section_headers[i].sect_size += ( long int ) sect_size[0];

		if ( header->section_headers[i].sect_type != 73 && header->section_headers[i].sect_type != 67 && header->section_headers[i].sect_type != 16 && header->section_headers[i].sect_type != 85 &&
			 header->section_headers[i].sect_type != 61 )
		{
			free ( header->section_headers );
			return 6;
		}

		if ( header->section_headers[i].sect_offset + header->header_size > file_size ( path ) )
		{
			free ( header->section_headers );
			return 7;
		}

		if ( header->section_headers[i].sect_size > file_size ( path ) )
		{
			free ( header->section_headers );
			return 8;
		}
	}

	close ( fd );
	return 0;
}

void print_parse_error ( int error_code )
{
	switch ( error_code )
	{
		case 0:
		{
			printf ( "SUCCESS\n" );
			return;
		}
		case 1:
		{
			printf ( "ERROR\n" );
			perror ( "" );
			exit ( LOCATION_NOT_ACCESSIBLE );
		}
		case 2:
		{
			printf ( "ERROR\ngiven file is too small to comply with SF format\n" );
			break;
		}
		case 3:
		{
			printf ( "ERROR\nwrong magic\n" );
			break;
		}
		case 4:
		{
			printf ( "ERROR\nwrong version\n" );
			break;
		}
		case 5:
		{
			printf ( "ERROR\nwrong sect_nr\n" );
			break;
		}
		case 6:
		{
			printf ( "ERROR\nwrong sect_types\n" );
			break;
		}
		case 7:
		{
			printf ( "ERROR\nwrong sect_offset\n" );
			break;
		}
		case 8:
		{
			printf ( "ERROR\nwrong sect_size\n" );
			break;
		}
	}

	exit ( DOES_NOT_COMPLY );
}

void print_header ( SF_header *header )
{
	short i;

	printf ( "version=%d\n", header->version );
	printf ( "nr_sections=%d\n", header->no_of_sections );

	for ( i = 0; i < ( short ) header->no_of_sections; i++ )
		printf ( "section%d: %s %d %ld\n", i + 1, header->section_headers[i].name, header->section_headers[i].sect_type, header->section_headers[i].sect_size );
}

void prepare_extract_parameters ( extract_parameters *extract_param, int argc, char **argv )
{
	if ( strncmp ( argv[2], "path=", 5 ) || strlen ( argv[2] ) < 6 )
	{
		USAGE_ERROR_LABEL_EXTRACT:
		printf ( "ERROR\nUSAGE: ./a1 extract path=<file_path> section=<sect_nr> line=<line_nr>\n" );
		exit ( USAGE_ERROR );
	}

	if ( strncmp ( argv[3], "section=", 8 ) || strlen ( argv[3] ) < 9 )
		goto USAGE_ERROR_LABEL_EXTRACT;

	if ( strncmp ( argv[4], "line=", 5 ) || strlen ( argv[3] ) < 6 )
		goto USAGE_ERROR_LABEL_EXTRACT;

	if ( strlen ( argv[2] + 5 ) > 1024 )
	{
		printf ( "ERROR\nmax path length exceeded\n" );
		exit ( PATH_TOO_LONG );
	}

	strcpy ( extract_param->path, argv[2] + 5 );

	extract_param->sect_nr = atoi ( argv[3] + 8 );

	if ( extract_param->sect_nr == 0 )
	{
		printf ( "ERROR\ninvalid section\n" );
		exit ( NaN );
	}

	extract_param->line = atoi ( argv[4] + 5 );

	if ( extract_param->line == 0 )
	{
		printf ( "ERROR\ninvalid line\n" );
		exit ( NaN );
	}
}

void extract ( extract_parameters *extract_param, SF_header *header )
{
	if ( access ( extract_param->path, F_OK ) )
	{
		printf ( "ERROR\ninvalid file\n" );
		exit ( LOCATION_NOT_ACCESSIBLE );
	}

	int fd = open ( extract_param->path, O_RDONLY );

	lseek ( fd, header->section_headers[extract_param->sect_nr - 1].sect_offset, SEEK_SET );

	char *section_buf = calloc ( header->section_headers[extract_param->sect_nr - 1].sect_size, sizeof ( char ) );
	read ( fd, section_buf, header->section_headers[extract_param->sect_nr - 1].sect_size );

	int current_line = 1;
	int cursor = 0;
	while ( current_line < extract_param->line )
	{
		if ( cursor < header->section_headers[extract_param->sect_nr - 1].sect_size )
		{
			if ( section_buf[cursor] == '\n' )
				current_line++;
			cursor++;
		}
		else
		{
			printf ( "ERROR\ninvalid line\n" );
			exit ( LINE_DOES_NOT_EXIST );
		}
	}

	int line_start = cursor;
	int line_length = 0;

	while ( section_buf[cursor] != 13 && section_buf[cursor] != 0 )
	{
		cursor++;
		line_length++;
	}

	printf ( "SUCCESS\n%.*s\n", line_length, section_buf + line_start );
	free ( section_buf );
}

void prepare_findall_parameters ( char *path, int argc, char **argv )
{
	if ( strncmp ( argv[2], "path=", 5 ) || strlen ( argv[2] ) < 6 )
	{
		printf ( "ERROR\nUSAGE: ./a1 findall path=file_path>\n" );
		exit ( USAGE_ERROR );
	}

	if( strlen( argv[2]) - 5 > 1023)
	{
		printf("ERROR\nmax path length exceeded\n");
		exit(PATH_TOO_LONG);
	}

	strcpy ( path, argv[argc - 1] + 5 );
}

int has_15_line_section ( char *path, SF_header *header )
{
	int fd = open(path, O_RDONLY);
	int current_section = 0;

	while(1)
	{
		if(current_section < header->no_of_sections)
		{
			lseek(fd, header->section_headers[current_section].sect_offset + 1, SEEK_SET);
			int no_lines = 1;
			int cursor = 0;
			char *section_buf = calloc(header->section_headers[current_section].sect_size, sizeof(char));
			read(fd, section_buf, header->section_headers[current_section].sect_size);

			while( section_buf[cursor] != 0 )
			{
				if( section_buf[cursor] == '\n' || section_buf[cursor] == 0 )
					no_lines++;
				cursor++;
			}

			if( no_lines == 15 ) {
				free(section_buf);
				return 0;
			}

			current_section++;
			free(section_buf);
		}
		else
			return 1;
	}
}

void findall ( char *path )
{
	if ( access( path, F_OK ) )
	{
		printf ( "ERROR\n" );
		perror ( "" );
		exit ( LOCATION_NOT_ACCESSIBLE );
	}

	struct stat stat_struct;
	stat(path, &stat_struct);

	if ( S_ISDIR ( stat_struct.st_mode ) == 0 )
	{
		printf ( "ERROR\ninvalid directory path\n" );
		exit ( NOT_A_DIRECTORY );
	}

	DIR *directory = opendir(path);
	struct dirent *entry;
	char entry_path[1024] = {0};

	while( ( entry = readdir( directory ) ) != NULL )
		if ( strcmp ( entry->d_name, "." ) && strcmp ( entry->d_name, ".." ) )
		{
			strcpy(entry_path, path);
			strcat(entry_path, "/");
			strcat(entry_path, entry->d_name);

			stat(entry_path, &stat_struct);
			if( S_ISDIR ( stat_struct.st_mode ) )
				findall(entry_path);
			else if ( S_ISREG ( stat_struct.st_mode ) )
			{
				SF_header header;
				
				if( parse(entry_path, &header) == 0 && has_15_line_section ( entry_path, &header ) == 0 )
					printf("%s\n", entry_path);
			}
		}

	closedir(directory);
}

int main ( int argc, char **argv )
{

	if ( argc == 1 )
	{
		printf ( "ERROR\nno arguments provided\n" );
		return NO_ARGUMENTS;
	}

	if ( strcmp ( argv[1], "variant" ) == 0 )
	{

		if ( argc != 2 )
		{
			printf ( "ERROR\nvariant accepts no parameters\n" );
			return TOO_MANY_PARAMETERS;
		}
		printf ( "50415\n" );

	}
	else if ( strcmp ( argv[1], "list" ) == 0 )
	{
		struct list_parameters list_param;

		if ( argc == 2 )
		{
			printf ( "ERROR\nlist requires at least a path parameter\n" );
			return TOO_FEW_PARAMETERS;
		}

		if ( argc > 5 )
		{
			printf ( "ERROR\nlist accepts at most 3 parameters\n" );
			return TOO_MANY_PARAMETERS;
		}

		sort_list_parameters ( argc, argv );
		prepare_list_parameters ( &list_param, argc, argv );

		list ( &list_param );

	}
	else if ( strcmp ( argv[1], "parse" ) == 0 )
	{
		char path[1024];
		SF_header header;

		if ( argc == 2 )
		{
			printf ( "ERROR\nparse requires a path parameter\n" );
			return TOO_FEW_PARAMETERS;
		}

		if ( argc > 3 )
		{
			printf ( "ERROR\nparse accepts only one path parameter\n" );
			return TOO_MANY_PARAMETERS;
		}

		prepare_parse_parameters ( path, argc, argv );
		print_parse_error ( parse ( path, &header ) );
		print_header ( &header );

		free ( header.section_headers );

	}
	else if ( strcmp ( argv[1], "extract" ) == 0 )
	{
		extract_parameters extract_param;
		SF_header header;

		if ( argc < 5 )
		{
			printf ( "ERROR\nparse requires 3 parameters\n" );
			return TOO_FEW_PARAMETERS;
		}

		if ( argc > 5 )
		{
			printf ( "ERROR\nparse requires 3 parameters\n" );
			return TOO_MANY_PARAMETERS;
		}

		prepare_extract_parameters ( &extract_param, argc, argv );
		int error_code = parse ( extract_param.path, &header );
		if(error_code)
			print_parse_error(error_code);
		else
			extract ( &extract_param, &header );

		free(header.section_headers);

	}
	else if ( strcmp(argv[1], "findall") == 0 )
	{
		if ( argc < 3 ) 
		{
			printf("ERROR\nfindall requires a path parameter\n");
			return TOO_FEW_PARAMETERS;
		}

		if( argc > 3)
		{
			printf("ERROR\nfinall accepts only one path parameter\n");
			return TOO_FEW_PARAMETERS;
		}

		char path[1024];

		prepare_findall_parameters( path, argc, argv);
		printf("SUCCESS\n");
		findall(path);
	}
	else
	{
		printf("ERROR\n\"%s\" command not recognised\n", argv[1]);
		return UNKNOWN_COMMAND;
	}

	return 0;
}