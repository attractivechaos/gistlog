use clap::Parser;
use std::process;
use std::time::Instant;

#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    /// Algorithm: 1=std::collections::HashMap, 2=hashbrown::HashMap
    #[arg(value_parser = clap::value_parser!(u32))]
    algorithm: u32,

    /// Number of elements (e.g. 19m, 500k)
    #[arg(short = 'n', default_value = "19m")]
    n: String,

    /// Reserve before merge
    #[arg(short = 'r', action = clap::ArgAction::SetTrue)]
    reserve: bool,
}

fn splitmix64(x: &mut u64) -> u64 {
    *x = x.wrapping_add(0x9e3779b97f4a7c15);
    let mut z = *x;
    z = (z ^ (z >> 30)).wrapping_mul(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27)).wrapping_mul(0x94d049bb133111eb);
    z ^ (z >> 31)
}

fn parse_num(s: &str) -> u64 {
    let mut end = s.len();
    let mut factor = 1.0;
    // Check for suffix
    if let Some(c) = s.chars().last() {
        if "gGmMkK".contains(c) {
            end -= 1;
            match c {
                'g' | 'G' => factor = 1e9,
                'm' | 'M' => factor = 1e6,
                'k' | 'K' => factor = 1e3,
                _ => {} 
            }
        }
    }
    
    let val_str = &s[..end];
    let val: f64 = val_str.parse().unwrap_or(0.0);
    (val * factor + 0.499) as u64
}

macro_rules! run_benchmark {
    ($map_type:ty, $label:expr, $n:expr, $reserve:expr) => {
        {
            let mut rng = 11;
            
            // h0 generation
            let mut h0 = <$map_type>::new();
            for _ in 0..$n {
                let x = splitmix64(&mut rng);
                *h0.entry(x).or_insert(0) += 1;
            }
            
            let t0 = Instant::now();
            
            // h1 generation
            let mut h1 = <$map_type>::new();
            for _ in 0..($n * 2) {
                let x = splitmix64(&mut rng);
                *h1.entry(x).or_insert(0) += 1;
            }
            
            let t1 = Instant::now();
            
            // Merging
            if $reserve {
                h0.reserve(h1.len());
            }
            for (k, v) in &h1 {
                *h0.entry(*k).or_insert(0) += v;
            }
            
            let t2 = Instant::now();
            
            let diff1 = t1.duration_since(t0).as_secs_f64();
            let diff2 = t2.duration_since(t1).as_secs_f64();
            
            println!("{}\t{:.3}\t{:.3}", $label, diff1, diff2);
        }
    };
}

fn main() {
    let args = Args::parse();
    
    let n = parse_num(&args.n);
    let reserve = args.reserve;

    match args.algorithm {
        1 => run_benchmark!(std::collections::HashMap<u64, u64>, "rust_std", n, reserve),
        2 => run_benchmark!(hashbrown::HashMap<u64, u64>, "hashbrown", n, reserve),
        _ => {
            eprintln!("Unknown algorithm: {}", args.algorithm);
            process::exit(1);
        }
    }
}