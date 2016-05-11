#include <iostream>
#include <iomanip>
using namespace std;
#include <string>
#include <sstream>
#include <windows.h>
#include <winscard.h>

string recieveNativeMessage();

// inProperty�F���M����f�[�^�̃v���p�e�B��
// inResult�F�������ʃR�[�h(0:ID���擾 1:�J�[�h�����o 2:�ُ�I��)
// inData�F���M����f�[�^���e
void sendNativeMessage(string inProperty, string inResult, string inData);

// inData�F��M�����f�[�^���e�̃o�C�g��
// inDataLen�F��M�f�[�^�̒���
int checkSwData(BYTE inData[], DWORD inDataLen);

void onSCardError(string inProperty, string inErrorMessage, SCARDCONTEXT inSCardContext, SCARDHANDLE inSCardHandle);

int main(int argc, char* argv[]) {

	//��������
	LONG RC = 0;

	// ���\�[�X�}�l�[�W���̃n���h��
	SCARDCONTEXT sCardContext = NULL;
	// ���[�_�^���C�^�̃n���h��
	SCARDHANDLE	sCardHandle = NULL;
	// ���[�_�^���C�^�̖���
	LPTSTR readerName;
	// �ʐM�Ɏg�p����v���g�R��
	DWORD activeProtocol;

	DWORD dwAutoAllocate = SCARD_AUTOALLOCATE;

	// chrome�g���@�\�ɑ��M����json�f�[�^�̃v���p�e�B�ƃ��b�Z�[�W
	string jsonProperty = "id";
	string jsonString = "";

	// ID�̒���
	int idLength = 0;
    // �J�[�h�̃^�O�^�C�v
	int cardTagType;



	// ���\�[�X�}�l�[�W���̃n���h�����擾
	RC = SCardEstablishContext( SCARD_SCOPE_USER, NULL, NULL, &sCardContext );
	if(RC != SCARD_S_SUCCESS){onSCardError( jsonProperty, "EstablishContext", sCardContext, sCardHandle); return 1;}

	// ���[�_�^���C�^�̖��O���擾�i�ڑ�����Ă��郊�[�_�^���C�^��1�̑O��j
	RC = SCardListReaders( sCardContext, NULL, (LPTSTR)&readerName, &dwAutoAllocate );
	if(RC != SCARD_S_SUCCESS){onSCardError( jsonProperty, "ListReaders", sCardContext, sCardHandle); return 1;}

	// ���[�_�^���C�^�̏����i�[����ϐ�
	SCARD_READERSTATE sReaderState;

	// ���[�_�^���C�^�̖��O���i�[
    sReaderState.szReader = readerName;
	// ���[�_�^���C�^�̏�Ԃ��i�[
    sReaderState.dwCurrentState = SCARD_STATE_UNAWARE;
    sReaderState.dwEventState = SCARD_STATE_UNAWARE;

	// chrome�g���@�\����W�����͂ɂ�郁�b�Z�[�W��҂��󂯂�
	// recieveNativeMessage�֐��̌��ʂ���̏ꍇ��chrome�g���@�\���I�������Ɣ��f����
	while(!(jsonString = recieveNativeMessage()).empty()){

		// �g���@�\����'close'������M�����ꍇ�̓��[�v���I������
		if(!jsonString.substr(9,5).compare("close")){break;}


		// ���[�_�^���C�^�̏�Ԃ��擾����
		RC = SCardGetStatusChange(sCardContext, 30, &sReaderState, 1);
		if(RC != SCARD_S_SUCCESS){onSCardError( jsonProperty, "GetStatusChange", sCardContext, sCardHandle); return 2;}

		// �J�[�h�����[�_�^���C�^�ɂ�������Ă���ΐڑ��������J�n����
		if((sReaderState.dwEventState & SCARD_STATE_PRESENT) == SCARD_STATE_PRESENT){

			// �J�[�h�Ƃ̒ʐM���J�n����
			RC = SCardConnect( sCardContext, readerName, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &sCardHandle, &activeProtocol);

			//�����ŃG���[���������ꍇ�A�v���g�R�����Ή��s�̎�ނ̏ꍇ�̓G���[���b�Z�[�W��chrome�g���@�\�ɑ��M���ďI������
			if(RC != SCARD_S_SUCCESS){onSCardError( jsonProperty, "Connect", sCardContext, sCardHandle); return 2;}
			else if(activeProtocol == SCARD_PROTOCOL_UNDEFINED){onSCardError( jsonProperty, "UndefinedProtocol", sCardContext, sCardHandle); return 2;}

			// ATR(Answer To Reset)���擾���J�[�h��ʂ𔻒肷��
			DWORD readerLength = sizeof( readerName );
			DWORD state;
			DWORD atrLength;
			BYTE  atr[64];

			// �g���@�\�ɑ��M����f�[�^���i�[����stringstream
			std::stringstream dataStream;

			// �J�[�h�Ƃ̒ʐM���J�n����
			RC = SCardStatus( sCardHandle, NULL, &readerLength, &state, &activeProtocol, NULL, &atrLength );
			if(RC != SCARD_S_SUCCESS){onSCardError( jsonProperty, "GetATR", sCardContext, sCardHandle);	return 2;}

			RC = SCardStatus( sCardHandle, readerName, &readerLength, &state, &activeProtocol, atr, &atrLength );
			if(RC != SCARD_S_SUCCESS){onSCardError( jsonProperty, "GetATR", sCardContext, sCardHandle);	return 2;}

			// �J�[�h�̎�ނ𔻒肵�Ď擾����ID�̒��������肷��
			if(atr[13] == 0x00){
		
				// FeliCa�EFeliCaLite-S�J�[�h�̏ꍇ��IDm�̒�����8�o�C�g
				if( atr[14] == 0x3b){

					idLength = 8;
					cardTagType = 3;

				// Mifare 1K�E4K�EUltraLight�J�[�h�̏ꍇ��UID�̒�����7�o�C�g
				} else if(atr[14] == 0x01 || atr[14] == 0x02 || atr[14] == 0x03){

					idLength = 7;
					cardTagType = 2;

				}

			// ��L�ȊO�̃T�|�[�g���Ă��Ȃ��J�[�h�̏ꍇ�̓G���[���b�Z�[�W��chrome�g���@�\�ɑ��M���ďI������
			} else { onSCardError( jsonProperty, "UnsupportedCard", sCardContext, sCardHandle); return 2;}

			// �g���@�\����̖��ߓ��e�𔻒肷��
			if(jsonString.substr(9,5).compare("getId") == 0){
//			if(false){

				// ID�擾�R�}���h
				BYTE getIdCommand[] = {0xFF, 0xCA, 0x00, 0x00, 0x00}; 
				// ��M�f�[�^���i�[����o�b�t�@����уo�b�t�@�T�C�Y
				BYTE recieveBuffer[8 + 2/* SW1+SW2 */];	
				DWORD recieveBufferSize = sizeof(recieveBuffer);

				// �J�[�h�ɃR�}���h���M���ăf�[�^����M����
				RC = SCardTransmit( sCardHandle, SCARD_PCI_T1, getIdCommand, sizeof(getIdCommand), NULL, recieveBuffer, &recieveBufferSize );
				if(RC != SCARD_S_SUCCESS){onSCardError( jsonProperty, "TransmitCommand", sCardContext, sCardHandle); return 2;}

		
				// ��M�f�[�^����16�i���ɕϊ����A�J�[�h��ʖ���ID�Ƃ���dataStream�Ɋi�[����
				for(int i = 0; i < idLength; i++){

					dataStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(recieveBuffer[i]);

				}

				::sendNativeMessage("id", "0", dataStream.str());

				break;


			}else if(jsonString.substr(9,7).compare("getNDEF") == 0){
//			}else if(true){

				// Felica(NFC Forum Type 3 Tag)�̏ꍇ
				if(cardTagType == 3){


					// ��M�f�[�^���i�[����o�b�t�@����уo�b�t�@�T�C�Y
					BYTE recieveSrvBuffer[16 + 2];	
					DWORD recieveSrvBufferSize = sizeof(recieveSrvBuffer);

					// �T�[�r�X�w��R�}���h
					BYTE srvCommand[] = {0xFF, 0xA4, 0x00, 0x01, 0x02, 0x0B, 0x00}; 

					// �J�[�h�ɃR�}���h���M���ăf�[�^����M����i��M�o�b�t�@��Block0�Ƌ��p�j
					RC = SCardTransmit( sCardHandle, SCARD_PCI_T1, srvCommand, sizeof(srvCommand), NULL, recieveSrvBuffer, &recieveSrvBufferSize );
					if(RC != SCARD_S_SUCCESS){onSCardError( jsonProperty, "ServiceCommand", sCardContext, sCardHandle);	return 2;}
					
					// ��M�f�[�^���i�[����o�b�t�@����уo�b�t�@�T�C�Y
					BYTE recieveBlk0Buffer[16 + 2];	
					DWORD recieveBlk0BufferSize = sizeof(recieveBlk0Buffer);

					// Block0�ւ�read�R�}���h
					BYTE readBlk0Command[] = {0xFF, 0xB0, 0x00, 0x00, 0x00}; 

					// �J�[�h�ɃR�}���h���M���ăf�[�^����M����
					RC = SCardTransmit( sCardHandle, SCARD_PCI_T1, readBlk0Command, sizeof(readBlk0Command), NULL, recieveBlk0Buffer, &recieveBlk0BufferSize );
					if(RC != SCARD_S_SUCCESS || checkSwData(recieveBlk0Buffer, recieveBlk0BufferSize)){onSCardError( jsonProperty, "ReadBlk0Command", sCardContext, sCardHandle); return 2;}

					// NDEF���b�Z�[�W�̒���
					int NDEFMsgLen = (static_cast<int>(recieveBlk0Buffer[11]) * 256 * 256) + (static_cast<int>(recieveBlk0Buffer[12]) * 256) + (static_cast<int>(recieveBlk0Buffer[13]));
					// NDEF���b�Z�[�W���g�p����u���b�N��
					int NDEFBlockLen = (NDEFMsgLen / 16) + 1;

					// NDEF�f�[�^���i�[����o�b�t�@�i�ǂݎ��4�u���b�N�̊i�[�o�C�g��(4blocks * 16bytes) + SW1 +SW2�j
					BYTE recieveNDEFBuffer[64 + 2]; 	
					DWORD recieveNDEFBufferSize = sizeof(recieveNDEFBuffer);

					// �ő�u���b�N������Read���߂����s
					for (int j = 0; j < NDEFBlockLen; j = j + 4){

						BYTE readNDEFCommand[] = {0xFF, 0xB0, 0x80, 0x04, 0x08, 0x80, j+1, 0x80, j+2, 0x80, j+3, 0x80, j+4, 0x40}; 

						// �J�[�h�ɃR�}���h���M���ăf�[�^����M����
						RC = SCardTransmit( sCardHandle, SCARD_PCI_T1, readNDEFCommand, sizeof(readNDEFCommand), NULL, recieveNDEFBuffer, &recieveNDEFBufferSize );
						if(RC != SCARD_S_SUCCESS || checkSwData(recieveNDEFBuffer, recieveNDEFBufferSize)){onSCardError( jsonProperty, "ReadNDEFCommand", sCardContext, sCardHandle); return 2;}

						for(int k = 0; k < 64; k++){

							dataStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(recieveNDEFBuffer[k]);

						}

					}

					::sendNativeMessage("NDEFMsg", "0", dataStream.str().substr(0, NDEFMsgLen * 2));

					break;


				}


			} else {
			
				::sendNativeMessage(jsonProperty, "2", "InvalidCommandRecieved");			    
			
			}

			// �J�[�h�Ƃ̒ʐM��ؒf
			SCardDisconnect( sCardHandle, SCARD_LEAVE_CARD );


		} else {
		
			::sendNativeMessage(jsonProperty, "1", "NoCardFound");
	
		}
		
			

	}

	// ���\�[�X�}�l�[�W���̃n���h�������
	SCardReleaseContext( sCardContext );

	return RC;

}


string recieveNativeMessage(){

	//���b�Z�[�W��
	unsigned int jsonLength = 0;

	//JSON�`���̃��b�Z�[�W
	string jsonMessage = "";

	// �擪4�o�C�g���烁�b�Z�[�W�̒������擾����
	jsonLength = getchar();

	// �擪1�o�C�g�ڂ�EOF(-1)�̏ꍇ��chrome�g���@�\�����I���������͂���̃��b�Z�[�W��Ԃ�
	if(jsonLength == EOF){

		return jsonMessage;

	}

	for (int i = 0; i < 3; i++){

		jsonLength += getchar();

	}


	//�擾����������json�`���̃��b�Z�[�W��jsonString�Ɋi�[����
	for (int i = 0; i < jsonLength; i++){

		jsonMessage += getchar();

	}

	return jsonMessage;

};

void sendNativeMessage(string inProperty, string inResult, string inMessage){

	// ���M���b�Z�[�W��json�`���ɕϊ�����
    std::string jsonString = "{\"result\": \"";
	jsonString.append(inResult).append( "\", \"");
	jsonString.append(inProperty).append("\": \"").append(inMessage).append( "\"}");
   
	// ���M���b�Z�[�W�̒������擾����
    unsigned int dataLength = jsonString.length();

    //���M���b�Z�[�W�̒�����4�o�C�g�̐����ŏo�͂���
    std::cout 
        << char(((dataLength >> 0) & 0xFF))
        << char(((dataLength >> 8) & 0xFF))
        << char(((dataLength >> 16) & 0xFF))
        << char(((dataLength >> 24) & 0xFF));

	// json�`���̑��M���b�Z�[�W���o��
	std::cout << jsonString;

}

int checkSwData(BYTE inData[], DWORD inDataLen){

    //�Ō����2�o�C�g(SW1�ESW2)��0x90�A0x00�ł���ΐ���Ɏ�M���Ă���
	if((inData[inDataLen - 2] == 0x90) && (inData[inDataLen - 1] == 0x00)){
	
		return 0;
	
	} else {

		return 1;

	}

}

void onSCardError(string inProperty, string inErrorMessage, SCARDCONTEXT inSCardContext, SCARDHANDLE inSCardHandle){

	string errorMesage =  "Error occured : ";

	errorMesage.append(inErrorMessage);

	// ���[�_�Ƃ̒ʐM��ؒf����
	SCardDisconnect(inSCardHandle, SCARD_LEAVE_CARD);
	// ���\�[�X�}�l�[�W���̃n���h�������
	SCardReleaseContext( inSCardContext );

	::sendNativeMessage(inProperty, "2", errorMesage);

}

