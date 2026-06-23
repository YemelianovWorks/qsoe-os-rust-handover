use std::env;
use std::fs;
use std::process;

use qsoe_qrvfs::Image;

fn main() {
    let mut args = env::args();
    let prog = args.next().unwrap_or_else(|| "qrvfs-tree".to_owned());
    let Some(path) = args.next() else {
        eprintln!("Usage: {prog} <qrvfs-image>");
        process::exit(2);
    };

    if args.next().is_some() {
        eprintln!("Usage: {prog} <qrvfs-image>");
        process::exit(2);
    }

    let bytes = match fs::read(&path) {
        Ok(bytes) => bytes,
        Err(err) => {
            eprintln!("{path}: {err}");
            process::exit(1);
        }
    };

    let image = match Image::parse(&bytes) {
        Ok(image) => image,
        Err(err) => {
            eprintln!("qrvfs-tree: {err}");
            process::exit(1);
        }
    };

    match image.format_tree(&path) {
        Ok(tree) => print!("{tree}"),
        Err(err) => {
            eprintln!("qrvfs-tree: {err}");
            process::exit(1);
        }
    }
}
