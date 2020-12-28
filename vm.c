// vm.c
enum{ VM_OP_INT0=0, VM_OP_INT1, VM_OP_INT2,
	VM_OP_INT3,
	VM_OP_INT4,
	VM_OP_INT5,
	VM_OP_INT6,
	VM_OP_INT7,
	VM_OP_INT8,
	VM_OP_INT_1_BYTE,
	VM_OP_INT_2_BYTE,
	VM_OP_INT_3_BYTE,
	VM_OP_INT_4_BYTE,
	VM_OP_INT_5_BYTE,
	VM_OP_INT_6_BYTE,
	VM_OP_INT_7_BYTE,
	VM_OP_INT_8_BYTE,
	VM_OP_FLOAT,
	VM_OP_JUMP,
	VM_OP_CJUMP,
	VM_OP_PUSH_VAR_00,
	VM_OP_PUSH_VAR_01,
	VM_OP_PUSH_VAR_02,
	VM_OP_PUSH_VAR_03,
	VM_OP_PUSH_VAR_04,
	VM_OP_PUSH_VAR_05,
	VM_OP_PUSH_VAR_06,
	VM_OP_PUSH_VAR_07,
	VM_OP_PUSH_VAR_08,
	VM_OP_PUSH_VAR_09,
	VM_OP_PUSH_VAR_10,
	VM_OP_PUSH_VAR_11,
	VM_OP_PUSH_VAR_12,
	VM_OP_PUSH_VAR_13,
	VM_OP_PUSH_VAR_14,
	VM_OP_PUSH_VAR_15,
	VM_OP_PUSH_VAR_16,
	VM_OP_PUSH_VAR_17,
	VM_OP_PUSH_VAR_18,
	VM_OP_PUSH_VAR_19,
	VM_OP_PUSH_VAR_20,
	VM_OP_PUSH_VAR_21,
	VM_OP_PUSH_VAR_22,
	VM_OP_PUSH_VAR_23,
	VM_OP_PUSH_VAR_24,
	VM_OP_PUSH_VAR_25,
	VM_OP_PUSH_VAR_26,
	VM_OP_PUSH_VAR_27,
	VM_OP_PUSH_VAR_28,
	VM_OP_PUSH_VAR_29,
	VM_OP_PUSH_VAR_30,
	VM_OP_PUSH_VAR_31,
	VM_OP_STORE_VAR_00,
	VM_OP_STORE_VAR_01,
	VM_OP_STORE_VAR_02,
	VM_OP_STORE_VAR_03,
	VM_OP_STORE_VAR_04,
	VM_OP_STORE_VAR_05,
	VM_OP_STORE_VAR_06,
	VM_OP_STORE_VAR_07,
	VM_OP_STORE_VAR_08,
	VM_OP_STORE_VAR_09,
	VM_OP_STORE_VAR_10,
	VM_OP_STORE_VAR_11,
	VM_OP_STORE_VAR_12,
	VM_OP_STORE_VAR_13,
	VM_OP_STORE_VAR_14,
	VM_OP_STORE_VAR_15,
	VM_OP_STORE_VAR_16,
	VM_OP_STORE_VAR_17,
	VM_OP_STORE_VAR_18,
	VM_OP_STORE_VAR_19,
	VM_OP_STORE_VAR_20,
	VM_OP_STORE_VAR_21,
	VM_OP_STORE_VAR_22,
	VM_OP_STORE_VAR_23,
	VM_OP_STORE_VAR_24,
	VM_OP_STORE_VAR_25,
	VM_OP_STORE_VAR_26,
	VM_OP_STORE_VAR_27,
	VM_OP_STORE_VAR_28,
	VM_OP_STORE_VAR_29,
	VM_OP_STORE_VAR_30,
	VM_OP_STORE_VAR_31,
	VM_OP_LOAD_INDEX,
	VM_OP_STORE_INDEX,
	VM_OP_CALL0,
	VM_OP_CALL1,
	VM_OP_CALL2,
	VM_OP_CALL3,
	VM_OP_CALL4,
	VM_OP_CALL5,
	VM_OP_CALL6,
	VM_OP_CALL7,
	VM_OP_CALL8,
	VM_OP_PUSH_GLOBAL,
	VM_OP_STORE_GLOBAL,
	VM_OP_LOAD_INDEX_GLOBAL,
	VM_OP_STORE_INDEX_GLOBAL,
	VM_OP_PUSH_VAR_INC,
	VM_OP_PUSH_GLOBAL_INC,
	VM_OP_ADD,
	VM_OP_SUB,
	VM_OP_MULT,
	VM_OP_DIVI,
	VM_OP_MOD,
	VM_OP_BITWISE_OR,
	VM_OP_BITWISE_XOR,
	VM_OP_BITWISE_AND,
	VM_OP_RSHIFT,
	VM_OP_LSHIFT,
	VM_OP_LOGICAL_NOT,
	VM_OP_BITWISE_NOT,
	VM_OP_NEG,
	VM_OP_EQUALS,
	VM_OP_NOT_EQUALS,
	VM_OP_LT,
	VM_OP_GT,
	VM_OP_LTEQ,
	VM_OP_GTEQ,
	VM_OP_PUSH_LOCALS,
	VM_OP_PUSH_LOCAL_ARRAY,
	VM_OP_PRINT,
	VM_OP_RETURN = 255
};

typedef struct {
	Data *globals;
} Core;

static s64
readListInt(u8 *input, s64 len)
{
	u64 listInt=0;
	
	len-=1;
	for(; len >= 0; len-=1)
	{
		listInt += (((u64)(*input))<<(len*8));
		input+=1;
	}
	
	return (s64)listInt;
}

static u8 *
listWriteInt(u8 *out, s64 value)
{
	u64 raw = (u64)value;
	s64 x;
	u32 lz;
	
	if(raw <= 8)
	{
		*out = raw;
		out++;
		return out;
	}
	
	lz = __builtin_clzl(raw);
	
	lz/=8;
	
	x = 8 - lz;
	
	*out = x + VM_OP_INT8;
	out++;
	x--;
	for(; x>= 0; x--)
	{
		*out = (raw>>(x*8))&0xFF;
		out+=1;
	}
	return out;
}

static f64
readListFloat(u8 *input)
{
	Data data;
	u64 listInt=0;
	s64 len = 7;
	
	do{
		listInt += (((u64)(*input))<<(len*8));
		input+=1;
		len-=1;
	}while(len>=0);
	
	data.i = listInt;
	return data.d;
}

Data
VirtualMachine(
	//Core * restrict c,
	Data * restrict globals,
	u8   * restrict ip,
	Data * restrict sp,
	Data * restrict fp,
	Data            tos  )
{

//~ printf("in VM\n");
loop:
//~ printf("VM Instruction %d\n", *ip);

switch(*ip)
{
	case VM_OP_INT0 ... VM_OP_INT8:
	{
		*sp = tos;
		sp++;
		tos.i = *ip;
		ip += 1;
		
		goto loop;
	}
	
	case VM_OP_INT_1_BYTE ... VM_OP_INT_8_BYTE:
	{
		s64 byteLen = *ip - VM_OP_INT8;
		*sp = tos;
		sp++;
		tos.i = readListInt(ip+1, byteLen);
		//~ if(*ip == 0){
			//~ printf("pushed 0, previous value %ld\n", (sp-1)->i);
		//~ }
		ip += byteLen + 1;
		
		goto loop;
	}
	
	case VM_OP_FLOAT:
	{
		*sp = tos;
		sp++;
		tos.d = readListFloat(ip+1);
		ip += 9;
		goto loop;
	}
	
	case VM_OP_JUMP:
	{
		s16 jump = (((s16)(*(ip+1)))<<8)+(s16)*(ip+2);
		ip+= jump;
		goto loop;
	}
	
	case VM_OP_CJUMP:
	{
		s32 jump = (((s32)(*(ip+1)))<<8)+(s32)*(ip+2);
		u32 isTrue  = tos.i!=0;
		u32 isFalse = isTrue^1;
		jump = ((-isTrue)&3)+((-isFalse)&jump);
		sp--;
		tos = *sp;
		ip+= jump;
		goto loop;
	}
	
	case VM_OP_CALL0 ... VM_OP_CALL8:
	{
		Data * restrict tmpSp;
		Data * restrict tmpFp;
		s16 jump = (((s16)(*(ip+1)))<<8)+(s16)*(ip+2);
		u8 numOfParams = *ip - VM_OP_CALL0;
		*sp = tos;
		sp = sp + (numOfParams>0);
		// current stack pointer value
		// [TmpExpr1]  [*spBlank] TOS = arg1
		tmpSp = sp;
		// reset stack pointer to where it was for current function
		// [*spTmpExpr1]  [Blank]  TOS = arg1
		sp-=numOfParams&0x07;
		// set new frame pointer to one above this
		//~ tmpFp = sp+1;
		tmpFp = sp;
		//~ printf("fp[0] = %ld\n", tmpFp[0].i);
		// move ip
		ip+=3;
		tos = VirtualMachine(globals, ip+jump, tmpSp, tmpFp, tos);
		goto loop;
	}
	
	case VM_OP_PUSH_VAR_00 ... VM_OP_PUSH_VAR_31:
	{
		*sp = tos;
		sp++;
		//~ printf("PUSH offset = %d, value = %ld, before = %ld, after = %ld\n", *ip - VM_OP_PUSH_VAR_00, fp[*ip - VM_OP_PUSH_VAR_00].i, fp[*ip - VM_OP_PUSH_VAR_00-1].i, fp[*ip - VM_OP_PUSH_VAR_00+1].i);
		tos = fp[*ip - VM_OP_PUSH_VAR_00];
		ip += 1;
		goto loop;
	}
	
	case VM_OP_STORE_VAR_00 ... VM_OP_STORE_VAR_31:
	{
		//~ printf("STORE offset = %d, value = %ld, before = %ld, after = %ld\n", *ip - VM_OP_STORE_VAR_00, fp[*ip - VM_OP_STORE_VAR_00].i, fp[*ip - VM_OP_STORE_VAR_00-1].i, fp[*ip - VM_OP_STORE_VAR_00+1].i);
		fp[*ip - VM_OP_STORE_VAR_00] = tos;
		sp--;
		tos = *sp;
		ip += 1;
		goto loop;
	}
	
	case VM_OP_LOAD_INDEX:
	{
		tos = fp[ip[1]].a[tos.i];
		ip += 2;
		goto loop;
	}
	
	case VM_OP_STORE_INDEX:
	{
		sp -= 2;
		fp[ip[1]].a[tos.i] = sp[1];
		tos = sp[0];
		ip += 2;
		goto loop;
	}
	
	case VM_OP_PUSH_GLOBAL:
	{
		*sp = tos;
		sp++;
		tos = globals[ip[1]];
		ip += 2;
		goto loop;
	}
	
	case VM_OP_STORE_GLOBAL:
	{
		globals[ip[1]] = tos;
		sp--;
		tos = *sp;
		ip += 2;
		goto loop;
	}
	
	case VM_OP_LOAD_INDEX_GLOBAL:
	{
		Data *array;
		array = &globals[globals[ip[1]].i];
		tos = array[tos.i];
		ip += 2;
		goto loop;
	}
	
	case VM_OP_STORE_INDEX_GLOBAL:
	{
		//~ printf("VM_OP_STORE_INDEX_GLOBAL\n");
		//~ printf("globals = %ld\n", (u64)globals);
		//~ printf("tos.i = %ld\n", tos.i);
		//~ printf("sp[1] = %ld\n", sp[1].i);
		Data *array;
		sp -= 2;
		array = &globals[globals[ip[1]].i];
		array[tos.i] = sp[1];
		tos = sp[0];
		ip += 2;
		//~ printf("global array set\n");
		goto loop;
	}
	
	case VM_OP_PUSH_VAR_INC:
	{
		*sp = tos;
		sp++;
		tos.i = fp[ip[1]].i;
		fp[ip[1]].i = tos.i+1;
		ip += 2;
		goto loop;
	}
	break;
	
	case VM_OP_PUSH_GLOBAL_INC:
	{
		*sp = tos;
		sp++;
		tos.i = globals[ip[1]].i;
		globals[ip[1]].i = tos.i+1;
		ip += 2;
		goto loop;
	}
	break;
	
	case VM_OP_ADD:
	{
		sp--;
		tos.i = sp->i + tos.i;
		ip++;
		goto loop;
	}
	
	case VM_OP_SUB:
	{
		sp--;
		tos.i = sp->i - tos.i;
		ip++;
		goto loop;
	}
	
	case VM_OP_MULT:
	{
		sp--;
		tos.i = sp->i * tos.i;
		ip++;
		goto loop;
	}
	
	case VM_OP_DIVI:
	{
		sp--;
		tos.i = sp->i / tos.i;
		ip++;
		goto loop;
	}
	
	case VM_OP_MOD:
	{
		sp--;
		tos.i = sp->i % tos.i;
		ip++;
		goto loop;
	}
	
	case VM_OP_NEG:
	{
		tos.i = -tos.i;
		ip++;
		goto loop;
	}
	
	case VM_OP_LOGICAL_NOT:
	{
		tos.i = !tos.i;
		ip++;
		goto loop;
	}

	case VM_OP_BITWISE_OR:
	{
		sp--;
		tos.i = sp->i | tos.i;
		ip++;
		goto loop;
	}
	
	case VM_OP_BITWISE_XOR:
	{
		sp--;
		tos.i = sp->i ^ tos.i;
		ip++;
		goto loop;
	}
	
	case VM_OP_BITWISE_AND:
	{
		sp--;
		tos.i = sp->i & tos.i;
		ip++;
		goto loop;
	}

	case VM_OP_RSHIFT:
	{
		sp--;
		tos.i = sp->i >> tos.i;
		ip++;
		goto loop;
	}

	case VM_OP_LSHIFT:
	{
		sp--;
		tos.i = sp->i << tos.i;
		ip++;
		goto loop;
	}

	case VM_OP_BITWISE_NOT:
	{
		tos.i = ~tos.i;
		ip++;
		goto loop;
	}
	
	case VM_OP_EQUALS:
	{
		sp--;
		tos.i = sp->i == tos.i;
		ip++;
		goto loop;
	}
	
	case VM_OP_NOT_EQUALS:
	{
		sp--;
		tos.i = sp->i != tos.i;
		ip++;
		goto loop;
	}
	
	case VM_OP_LT:
	{
		sp--;
		tos.i = sp->i < tos.i;
		ip++;
		goto loop;
	}
	
	case VM_OP_GT:
	{
		sp--;
		tos.i = sp->i > tos.i;
		ip++;
		goto loop;
	}
	
	case VM_OP_LTEQ:
	{
		sp--;
		tos.i = sp->i <= tos.i;
		ip++;
		goto loop;
	}
	
	case VM_OP_GTEQ:
	{
		sp--;
		tos.i = sp->i >= tos.i;
		ip++;
		goto loop;
	}
	
	case VM_OP_PUSH_LOCALS:
	{
		sp+=ip[1];
		ip+=2;
		goto loop;
	}
	
	case VM_OP_PUSH_LOCAL_ARRAY:
	{
		s64 sizeOfArray = tos.i;
		sp--;
		fp[ip[1]].a = sp;
		sp+= sizeOfArray;
		ip += 2;
		goto loop;
	}
	
	case VM_OP_PRINT:
	{
		sp--;
		printf("%ld\n", tos.i);
		printf("%ld\n", (u64)sp);
		tos = *sp;
		ip++;
		goto loop;
	}
	
	case VM_OP_RETURN:
	{
		return tos;
	}
	
	// unrecognized byte code
	default:
	ip++;
	goto loop;
}

}
