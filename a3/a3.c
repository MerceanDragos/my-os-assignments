#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>

typedef struct section_header {
    char name[14];
    short sect_type;
    long int sect_offset;
    long int sect_size;
} section_header;

typedef struct SF_header {
    char version;
    char no_of_sections;
    section_header *section_headers;
    int header_size;
    char magic[2];
} SF_header;

typedef struct shm_struct {
    char *addr;
    unsigned int size;
} shm_struct;

shm_struct shm;
shm_struct shm_file;
SF_header header;

void ping ( int resp ) {
    write ( resp, "PING$PONG$", 10 );
    unsigned int n = 50415;
    write ( resp, &n, sizeof ( unsigned int ) );
}

void create_shm ( int resp, int req ) {

    read ( req, &( shm.size ), sizeof ( unsigned int ) );

    int shm_fd = shm_open ( "/il9AUd", O_CREAT | O_RDWR, 0664 );

    ftruncate ( shm_fd, shm.size );

    shm.addr = ( char * ) mmap ( NULL, shm.size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0 );

    close ( shm_fd );

    write ( resp, "CREATE_SHM$SUCCESS$", 19 );
}

void write_to_shm ( int resp, int req ) {

    unsigned int offset;

    read ( req, &offset, sizeof ( unsigned int ) );

    unsigned int value;

    read ( req, &value, sizeof ( unsigned int ) );

    if ( offset + sizeof ( unsigned int ) < shm.size ) {
        memcpy ( shm.addr + offset, &value, sizeof ( unsigned int ) );
        write ( resp, "WRITE_TO_SHM$SUCCESS$", 21 );
    } else {
        write ( resp, "WRITE_TO_SHM$ERROR$", 19 );
    }
}

void map_file ( int resp, int req ) {

    int cursor = 0;
    char c;
    char file[250] = { 0 };

    while ( read ( req, &c, 1 ) == 1 && c != '$' ) {
        file[cursor] = c;
        cursor++;
    }
    file[cursor] = '\0';

    int fd;

    if ( ( fd = open ( file, O_RDONLY ) ) != -1 ) {
        shm_file.size = lseek ( fd, 0, SEEK_END );
        shm_file.addr = ( char * ) mmap ( NULL, shm_file.size, PROT_READ, MAP_SHARED, fd, 0 );
        write ( resp, "MAP_FILE$SUCCESS$", 17 );
    } else {
        write ( resp, "MAP_FILE$ERROR$", 15 );
    }
}

void read_from_file_offset ( int resp, int req ) {

    unsigned int offset;

    read ( req, &offset, sizeof ( unsigned int ) );

    unsigned int no_bytes;

    read ( req, &no_bytes, sizeof ( unsigned int ) );

    if ( offset + no_bytes < shm_file.size ) {
        memcpy ( shm.addr, shm_file.addr + offset, no_bytes );
        write ( resp, "READ_FROM_FILE_OFFSET$SUCCESS$", 30 );
    } else
        write ( resp, "READ_FROM_FILE_OFFSET$ERROR$", 28 );
}

int parse ( ) {
    int cursor = shm_file.size - 4;
    unsigned char header_size_str[2];

    memcpy ( header_size_str, shm_file.addr + cursor, 2 );
    cursor += 2;
    memcpy ( header.magic, shm_file.addr + cursor, 2 );
    cursor += 2;
    
    if ( strncmp ( header.magic, "Nu", 2 ) )
        return 3;

    header.header_size = 0;
    header.header_size += ( int ) header_size_str[1] * 256;
    header.header_size += ( int ) header_size_str[0];

    cursor = shm_file.size - header.header_size;

    memcpy ( &( header.version ), shm_file.addr + cursor, 1 );
    cursor++;
    if ( header.version < 15 || header.version > 51 )
        return 4;

    memcpy ( &( header.no_of_sections ), shm_file.addr + cursor, 1 );
    cursor++;
    if ( header.no_of_sections < 3 || header.no_of_sections > 19 )
        return 5;

    int i = 0;
    unsigned char sect_type[2];
    unsigned char sect_offset[4];
    unsigned char sect_size[4];
    header.section_headers = calloc ( header.no_of_sections, sizeof ( section_header ) );

    for ( ; i < ( int ) header.no_of_sections; i++ ) {
        memcpy ( header.section_headers[i].name, shm_file.addr + cursor, 13 );
        cursor += 13;
        memcpy ( sect_type, shm_file.addr + cursor, 2 );
        cursor += 2;
        memcpy ( sect_offset, shm_file.addr + cursor, 4 );
        cursor += 4;
        memcpy ( sect_size, shm_file.addr + cursor, 4 );
        cursor += 4;

        header.section_headers[i].sect_type += ( int ) sect_type[1] * 256;
        header.section_headers[i].sect_type += ( int ) sect_type[0];

        header.section_headers[i].sect_offset += ( long int ) sect_offset[3] * 16777216l;
        header.section_headers[i].sect_offset += ( long int ) sect_offset[2] * 65536l;
        header.section_headers[i].sect_offset += ( long int ) sect_offset[1] * 256l;
        header.section_headers[i].sect_offset += ( long int ) sect_offset[0];

        header.section_headers[i].sect_size += ( long int ) sect_size[3] * 16777216l;
        header.section_headers[i].sect_size += ( long int ) sect_size[2] * 65536l;
        header.section_headers[i].sect_size += ( long int ) sect_size[1] * 256l;
        header.section_headers[i].sect_size += ( long int ) sect_size[0];

        if ( header.section_headers[i].sect_type != 73 && header.section_headers[i].sect_type != 67 && header.section_headers[i].sect_type != 16 &&
             header.section_headers[i].sect_type != 85 &&
             header.section_headers[i].sect_type != 61 ) {
            free ( header.section_headers );
            return 6;
        }

        if ( header.section_headers[i].sect_offset + header.header_size > shm_file.size ) {
            free ( header.section_headers );
            return 7;
        }

        if ( header.section_headers[i].sect_size > shm_file.size ) {
            free ( header.section_headers );
            return 8;
        }
    }

    return 0;
}

void read_from_file_section ( int resp, int req ) {

    unsigned int section_no;

    read ( req, &section_no, sizeof ( unsigned int ) );

    unsigned int offset;

    read ( req, &offset, sizeof ( unsigned int ) );

    unsigned int no_of_bytes;

    read ( req, &no_of_bytes, sizeof ( unsigned int ) );

    int err = parse ( );

    if ( err == 0 && section_no <= header.no_of_sections && offset + no_of_bytes < header.section_headers[section_no - 1].sect_offset + header.section_headers[section_no - 1].sect_size && no_of_bytes < shm.size ) {
        memcpy ( shm.addr, shm_file.addr + header.section_headers[section_no - 1].sect_offset + offset, no_of_bytes );
        write ( resp, "READ_FROM_FILE_SECTION$SUCCESS$", 31 );
    } else {
        write ( resp, "READ_FROM_FILE_SECTION$ERROR$", 29 );
    }
}

void print_header ( )
{
	short i;
	printf ( "nr_sections=%d\n", header.no_of_sections );

	for ( i = 0; i < ( short ) header.no_of_sections; i++ )
		printf ( "section%d: %ld %ld\n", i + 1, header.section_headers[i].sect_offset,  header.section_headers[i].sect_size );
}

void read_from_logical_space_offset ( int resp, int req ) {

    unsigned int logical_offset;

    read ( req, &logical_offset, sizeof ( unsigned int ) );

    unsigned int no_of_bytes;

    read ( req, &no_of_bytes, sizeof ( unsigned int ) );

    int err = parse();

    if ( err ) {
        write(resp, "READ_FROM_LOGICAL_SPACE_OFFSET$ERROR$", 37);
        return;
    }

    int section_no = 1;

    while( section_no <= header.no_of_sections && logical_offset > 2048 ) {
        int section_size = header.section_headers[section_no - 1].sect_size;
        if( section_size > logical_offset )
            break;
        else {
            logical_offset -= 2048u * ( 1u + ( section_size / 2048u ) );
            section_no++;
        }
    }

    if ( section_no <= header.no_of_sections ){
        memcpy(shm.addr, shm_file.addr + header.section_headers[section_no - 1].sect_offset + logical_offset, no_of_bytes);
        write(resp, "READ_FROM_LOGICAL_SPACE_OFFSET$SUCCESS$", 39);
    }
    else
        write(resp, "READ_FROM_LOGICAL_SPACE_OFFSET$ERROR$", 37);


}

int main ( int argc, char **argv ) {

    int req, resp;

    if ( mkfifo ( "RESP_PIPE_50415", 0777 ) && !( errno == EEXIST ) )
        printf ( "ERROR\ncannot create the response pipe\n" );

    if ( ( req = open ( "REQ_PIPE_50415", O_RDONLY ) ) < 0 )
        printf ( "ERROR\ncannot open the request pipe\n" );

    if ( ( resp = open ( "RESP_PIPE_50415", O_WRONLY ) ) < 0 )
        printf ( "ERROR\ncannot open the resp pipe\n" );
    else if ( write ( resp, "HELLO$", 6 ) == 6 )
        printf ( "SUCCESS\n" );


    while ( 1 ) {
        int cursor = 0;
        char c;
        char buf[250] = { 0 };

        while ( read ( req, &c, 1 ) == 1 && c != '$' ) {
            buf[cursor] = c;
            cursor++;
        }

        if ( strcmp ( buf, "PING" ) == 0 )
            ping ( resp );
        else if ( strcmp ( buf, "CREATE_SHM" ) == 0 )
            create_shm ( resp, req );
        else if ( strcmp ( buf, "WRITE_TO_SHM" ) == 0 )
            write_to_shm ( resp, req );
        else if ( strcmp ( buf, "MAP_FILE" ) == 0 )
            map_file ( resp, req );
        else if ( strcmp ( buf, "READ_FROM_FILE_OFFSET" ) == 0 )
            read_from_file_offset ( resp, req );
        else if ( strcmp ( buf, "READ_FROM_FILE_SECTION" ) == 0 )
            read_from_file_section ( resp, req );
        else if ( strcmp (buf, "READ_FROM_LOGICAL_SPACE_OFFSET") == 0 )
            read_from_logical_space_offset( resp, req );
        else if ( strcmp ( buf, "EXIT" ) == 0 ) {
            munmap ( shm.addr, shm.size );
            munmap ( shm_file.addr, shm_file.size );
            free ( header.section_headers );
            break;
        }
    }

    close( req );
    close( resp );

    return 0;
}